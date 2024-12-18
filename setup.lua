
-- Designed for Lua 5.4.

local gcc_config=" --enable-languages=c,c++ --disable-multilib "

local fmt = string.format

function usage(error_message)
   print("Usage:")
   print(" setup.lua [options]")
   print("")
   print("Downloads and installs a given GCC version using a hardcoded folder hierarchy that ")
   print("permits multiple versions to live side-by-side. The plug-in project makefiles are ")
   print("set up to rely on the same folder hierarchy.")
   print("")
   print("Required options:")
   print(" -v, --version <a>.<b>.<c>       desired GCC version")
   print("")
   print("Additional options:")
   print(" -f, --fast                      fast build (w/o localization, bootstrapping, etc.)")
   print(" --erase-prerequisites           clean prerequisites first (e.g. to fix corrupt downloads)")
   print(" --just-fix-gmp                  get GMP in the build directory and do nothing else")
   print(" --threads <positive-integer>    override thread count (default is nproc i.e. all cores)")
   print("                                 (useful for building multiple GCCs at a time, since not ")
   print("                                 all build steps seem to parallelize well)")
   if error_message then
      print("")
      print("Error: " .. error_message)
   end
   os.exit(1)
end

--

-- Helper for shell and syscalls.
local shell = {
   cwd      = nil,
   cwd_real = nil,
}
do -- member functions
   function shell:cd(path)
      --
      -- We can't just use `os.execute` to run `cd` because that 
      -- spawns a separate shell/process. Instead, we have to 
      -- remember a preferred current working directory, so we 
      -- can prefix each command with a `cd` call as a one-liner.
      --
      if path then
         self.cwd      = nil
         self.cwd_real = self:run_and_read("readlink -f " .. path)
      else
         self.cwd_real = nil
      end
      self.cwd = path
   end
   
   function shell:resolve(path) -- resolve a relative path (e.g. ~/foo) to an absolute one
      local command = "readlink -f " .. path
      local stream  = assert(io.popen(command, "r"))
      local output  = stream:read("*all")
      stream:close()
      output = output:gsub("\n$", "")
      return output
   end
   
   function shell:exec(command) -- returns exit code, or nil if terminated
      if self.cwd then
         command = string.format("cd %s && %s", self.cwd, command)
      end
      local success, exit_type, exit_code = os.execute(command)
      if exit_type == "exit" then
         return exit_code
      end
      return nil
   end
   function shell:run(command) -- returns bool
      return self:exec(command) == 0
   end
   function shell:run_and_read(command) -- returns echo'd output
      if self.cwd then
         command = string.format("cd %s && %s", self.cwd, command)
      end
      local stream = assert(io.popen(command, "r"))
      local output = stream:read("*all")
      stream:close()
      output = output:gsub("\n$", "")
      return output
   end
   
   -- https://stackoverflow.com/a/40195356
   function shell:exists(name) -- file or folder
      return shell:run("stat " .. name .. " > /dev/null")
   end
   function shell:is_directory(path)
      return self:exists(path .. "/")
   end
   
   function shell:has_program(name) -- check if a program is installed
      return self:exec("command -v " .. name) == 0
   end
end

--

-- Helper for formatted text output.
local console = {
   stream = io.output()
}
do
   function _stringify_formatting(options)
      if not options then
         return "\x1b[0m"
      end
      local out = "\x1b["
      
      local empty = true
      function _write(s)
         if not empty then
            out = out .. ";"
         end
         out = out .. s
         empty = false
      end
      
      if options.reset then
         _write("0")
      end
      if options.bold then
         _write("1")
      end
      if options.back_color then
         _write(string.format(
            "48;2;%u;%u;%u",
            options.back_color.r or options.back_color[1] or 0,
            options.back_color.g or options.back_color[2] or 0,
            options.back_color.b or options.back_color[3] or 0
         ))
      end
      if options.text_color then
         _write(string.format(
            "38;2;%u;%u;%u",
            options.text_color.r or options.text_color[1] or 0,
            options.text_color.g or options.text_color[2] or 0,
            options.text_color.b or options.text_color[3] or 0
         ))
      end
      out = out .. "m"
      return out
   end

   function console:format(options)
      self:print(_stringify_formatting(options))
   end
   function console:print(text)
      self.stream:write(text)
   end
   
   local RED   = { 255,  64,  64 }
   local WHITE = { 240, 240, 240 }
   local BLUE  = {   0,   0, 128 }
   
   function _make_icon(glyph, back_color, glyph_color)
      local out = ""
      out = out .. _stringify_formatting({ text_color = back_color })
      out = out .. utf8.char(0x2590)
      out = out .. _stringify_formatting({ back_color = back_color, text_color = glyph_color })
      out = out .. glyph
      out = out .. _stringify_formatting({ reset = true, text_color = back_color })
      out = out .. utf8.char(0x258C)
      out = out .. _stringify_formatting()
      return out
   end
   
   local ERROR_ICON   = _make_icon("!", RED, WHITE)
   local ERROR_FORMAT = _stringify_formatting({ reset = true, text_color = RED })
   
   local STEP_ICON   = _make_icon("i", WHITE, BLUE)
   local STEP_FORMAT = _stringify_formatting({ reset = true, bold = true })
   
   local HIGHLIGHT_FORMAT = _stringify_formatting({ text_color = { 0, 255, 255 } })
   
   local RESET_FORMAT = _stringify_formatting()
   
   function console:report_error(text)
      self:print("\n")
      self:print(ERROR_ICON)
      self:print(ERROR_FORMAT)
      text = text:gsub("<h>",  HIGHLIGHT_FORMAT)
      text = text:gsub("</h>", ERROR_FORMAT)
      self:print(text)
      self:print(RESET_FORMAT)
      self:print("\n\n")
   end
   function console:report_fatal_error(text)
      self:report_error(text)
      os.exit(1)
   end
   function console:report_step(text)
      self:print("\n")
      self:print(STEP_ICON)
      self:print(STEP_FORMAT)
      text = text:gsub("<h>",  HIGHLIGHT_FORMAT)
      text = text:gsub("</h>", STEP_FORMAT)
      self:print(text)
      self:print(RESET_FORMAT)
      self:print("\n")
      -- Unlike errors, we don't print a second line break after because some 
      -- places may want to print additional information paired with this output.
   end
   
   function console:set_window_title(text)
      self.stream:write("\x1b]0;" .. text .. "\x1b\x5C")
   end
end

--

local params = {
   fast                = false,
   erase_prerequisites = false,
   just_fix_gmp        = false,
   threads             = shell:run_and_read("nproc"),
   version             = nil,
}
--
-- Extract command line arguments:
--
do
   local OPTION_ALIASES = {
      f = "fast",
      v = "version",
   }

   local size = #arg
   local key  = nil
   local raw  = {}
   for i = 1, size, 1 do
      local item   = arg[i]
      local is_key = false
      if item:sub(1, 2) == "--" then
         item = item:sub(3)
      elseif item[0] == "-" then
         item = item:sub(2)
      else
         if key then
            local list = raw[key]
            list[#list + 1] = item
         else
            usage()
         end
         goto continue
      end
      local j     = item:find("=", 1, true)
      local value = nil
      if j then
         key   = item:sub(1, j)
         value = item:sub(j + 1)
      else
         key = item
      end
      key = OPTION_ALIASES[key] or key
      if value then
         raw[key] = { value }
      else
         raw[key] = {}
      end
      ::continue::
   end
   
   if raw["erase-prerequisites"] then
      params.erase_prerequisites = true
   end
   if raw["fast"] then
      params.fast = true
   end
   if raw["just-fix-gmp"] then
      params.just_fix_gmp = true
   end
   do
      local value = raw["threads"]
      if value and value[1] then
         value = value[1]
         num   = tonumber(value)
         if not num then
            usage("option --threads, if specified, must be a positive integer (seen: " .. value .. " which isn't a number)")
         end
         if num <= 0 then
            usage("option --threads, if specified, must be a positive integer (seen: " .. value .. " which is zero or negative)")
         end
         if num ~= math.floor(num) then
            usage("option --threads, if specified, must be a positive integer (seen: " .. value .. " which is not an integer)")
         end
         params.threads = num
      end
   end
   do
      local value = raw["version"]
      if not value or not value[1] then
         usage("option --version not specified")
      end
      value = value[1]
      local match = value:match("^%d+%.%d+%.%d+$")
      if not match then
         usage("option --version does not resemble a version number (e.g. \"1.2.3\")")
      end
      params.version = value
   end
end

if params.fast then
   --[[
      --disable-nls
         Don't install localization files.
      
      --disable-dependency-tracking
         Don't track and cache dependency information that would be useless 
         for one-off builds of a program that I don't intend to modify.
      
      --disable-bootstrap
         Use a simpler build process, because we don't care whether the GCC 
         we're building here can build other GCCs.
      
      
      On my machine (WSL1 on an SSD), this cuts GCC's build time down from 
      45 minutes to about 11 minutes, not including the time it takes to 
      build GMP separately for those GCC versions that need it.
   ]]--
   gcc_config = gcc_config .. " --disable-nls --disable-dependency-tracking --disable-bootstrap"
end

function version_is_at_least(other)
   function _split(v)
      local list = {}
      for s in string.gmatch(v, "([^.]+)") do
         table.insert(list, tonumber(s))
      end
      return list
   end
   local a = _split(params.version)
   local b = _split(other)
   for i = 1, 3 do
      if a[i] < b[i] then
         return false
      end
   end
   return true
end

--

function ensure_gcc_build_essentials()
   local has_bzip2 = shell:has_program("bzip2")
   local has_be    = shell:has_program("make")
   if (not has_bzip2) or (not has_be) then
      console:set_window_title("Attempting to install needed programs for GCC...")
      console:report_step("Attempting to install needed programs for GCC...")
      print("")
   
      -- Update the locally stored list of available packages and 
      -- package versions. Both of these commands are required, in 
      -- my experience.
      shell:exec("sudo apt update")
      shell:exec("sudo apt-get update --fix-missing")
      
      if not has_bzip2 then
         if not shell:run("sudo apt-get install -y bzip2 --fix-missing") then
            console:report_fatal_error("Failed to set up bzip2.")
         end
      end
      if not has_be then
         if not shell:run("sudo apt-get install -y build-essential --fix-missing") then
            console:report_fatal_error("Failed to set up build-essential.")
         end
      end
   end
end
function ensure_m4()
   if not shell:has_program("m4") then
      console:set_window_title("Attempting to install M4 (required for GMP build)...")
      console:report_step("Attempting to install M4 (required for GMP build)...")
      print("")
      
      if not shell:run("sudo apt-get install -y m4") then
         console:report_fatal_error("Failed to set up M4.")
      end
   end
end

--

local PATHS = {
   base    = "~/gcc/",
   dl_base = "~/gcc/dl/",
   dl_ver  = "~/gcc/dl/gcc-" .. params.version .. "/",
   build   = "~/gcc/build/" .. params.version .. "/",
   install = "~/gcc/install/" .. params.version .. "/",
}

function deal_with_archive()
   shell:cd(PATHS.dl_base)
   local filename = fmt("gcc-%s.tar.gz", params.version)

   function download_archive()
      console:set_window_title(fmt("Downloading GCC %s...", params.version))
      console:report_step(fmt("Downloading GCC %s to <h>%s</h>...", params.version, PATHS.dl_base))
      print("")
      
      local result = shell:exec(fmt("wget 'http://ftp.gnu.org/gnu/gcc/gcc-%s/%s' --continue", params.version, filename))
      if result ~= 0 then
         local ERRORS = {
            [1] = "Download failed. Aborting installation process.",
            [3] = "Download failed. (File I/O issue.) Aborting installation process.",
            [4] = "Download failed. (Network failure.) Aborting installation process.",
            [5] = "Download failed. (SSL verification failure.) Aborting installation process.",
            [8] = "Download failed. (Server issued an error response; missing file?) Aborting installation process.",
         }
         result = ERRORS[result]
         if not result then
            result = "Download failed. Aborting installation process."
         end
         console:report_fatal_error(result)
      end
   end
   
   function unpack_archive(use_old_files)
      local tar_params = ""
      if use_old_files then
         tar_params = "--skip-old-files"
      end
      
      console:set_window_title(fmt("Unpacking GCC %s...", params.version))
      console:report_step(fmt("Unpacking GCC %s to <h>%sgcc-%s/</h>...", params.version, PATHS.dl_base, params.version))
      print("")
      
      local result = nil
      if not shell:has_program("pv") then
         tar_params = tar_params .. " --checkpoint-action=ttyout='(%d seconds elapsed): %T%*\r'"
         
         result = shell:exec(fmt("tar xf \"%s\" %s", filename, tar_params))
      else
         --
         -- If the `pv` command is available, then use it to show a progress 
         -- bar for the extraction. If we pass the TAR file as an argument 
         -- to `pv` and then pipe the output of `pv` to `tar`, the result is 
         -- that `pv` will be responsible for feeding the TAR file to `tar`, 
         -- and it'll measure how much of the file has been fed in.
         --
         result = shell:exec(fmt("pv \"%s\" | tar x %s", filename, tar_params))
      end
      return result == 0
   end
   
   local already_have_archive = false
   if shell:exists(filename) then
      console:report_step(fmt("You already have the archive for GCC %s downloaded to <h>%s%s</h>; skipping download.", params.version, PATHS.dl_base, filename))
      already_have_archive = true
   else
      download_archive()
   end
   if not unpack_archive() then
      if already_have_archive then
         console:report_error("Unpacking failed. Perhaps the download was interrupted? Re-downloading...")
         
         download_archive()
         if not unpack_archive(false) then
            console:report_fatal_error("Unpacking failed again. Aborting installation process.")
         end
      else
         console:report_fatal_error("Unpacking failed. Aborting installation process.")
      end
   end
end
if not params.just_fix_gmp then
   deal_with_archive()
end

function deal_with_prereqs()
   shell:cd(PATHS.dl_ver)
   if params.erase_prerequisites then
      do
         local status = fmt("Cleaning out (in-tree) prerequisites for GCC %s...", params.version)
         console:set_window_title(status)
         console:report_step(status)
         print("")
      end
      
      local FILENAME_PATTERNS = {
         "gettext-*.tar.*",
         "gmp-*.tar.*",
         "isl-*.tar.*",
         "mpc-*.tar.*",
         "mpfr-*.tar.*",
      }
      for name in FILENAME_PATTERNS do
         shell:exec(string.format("find . -name '%s' -exec rm {} \\;", name))
      end
   end
   
   ensure_gcc_build_essentials()
   do
      local status = fmt("Downloading (in-tree) prerequisites for GCC %s...", params.version)
      console:set_window_title(status)
      console:report_step(status)
      print("")
   end
   -- TODO: This should be shell:run but we should test that before committing it
   if not shell:exec("./contrib/download_prerequisites") then
      console:report_fatal_error("Failed to download prerequisites. Aborting installation process. Recommend retrying with --erase-prerequisites to clear any corrupt downloads.")
   end
end
if not params.just_fix_gmp then
   deal_with_prereqs()
end

function pull_gmp_into_build_dir()
   ensure_m4()
   
   shell:cd(PATHS.dl_ver)
   local gmp_dir = nil
   do
      local name = shell:run_and_read("find . -name 'gmp-*/' -printf '%P'")
      if not name then
         console:report_error("GMP not found.")
         return false
      end
      gmp_dir = PATHS.dl_ver + name
   end
   
   local dst_dir = fmt("%sgmp/", PATHS.build)
   
   console:set_window_title("Installing GMP headers...")
   console:report_step(fmt("Installing GMP headers to <h>%s</h>...", dst_dir))
   print("")
   
   shell:exec(fmt("mkdir -p %s", dst_dir))
   shell:cd(dst_dir)
   shell:exec(fmt("%sconfigure --prefix=%s", gmp_dir, shell:resolve(dst_dir)))
   shell:exec(fmt("make -j%d", params.threads))
end

function build_and_install_gcc()
   shell:exec(fmt("mkdir -p %s", PATHS.build))
   shell:exec(fmt("mkdir -p %s", PATHS.install))
   
   console:set_window_title(fmt("Building GCC %s...", params.version))
   console:report_step(fmt("Building GCC %s to <h>%s</h>...", params.version, PATHS.build))
   print("")
   
   shell:cd(PATHS.build)
   --[[
      Start by configuring the build process. This step will create the makefiles 
      that we'll use to build GCC.
   ]]--
print(fmt(
   "%sconfigure --prefix=%s %s",
   PATHS.dl_ver,
   shell:resolve(PATHS.install),
   gcc_config
))
   if not shell:run(fmt(
      "%sconfigure --prefix=%s %s",
      PATHS.dl_ver,
      shell:resolve(PATHS.install),
      gcc_config
   )) then
      console:report_fatal_error("Build configuration failed. Aborting installation process.")
   end
   --[[
      Run those makefiles.
   ]]--
   if not shell:run(fmt("make -j%d", params.threads)) then
      console:report_fatal_error("Build failed. Aborting installation process.")
   end
   
   console:set_window_title(fmt("Installing GCC %s...", params.version))
   console:report_step(fmt("Installing GCC %s to <h>%s</h>...", params.version, PATHS.install))
   print("")
   
   --[[
      DO NOT parallelize `make install`; it doesn't appear to be designed for that, 
      and failures have been reported e.g. 
      https://gcc.gnu.org/bugzilla/show_bug.cgi?id=42980
   ]]--
   if not shell:run("make install") then
      console:report_fatal_error("Installation failed. Aborting installation process.")
   end
   
   if params.fast and not version_is_at_least("13.1.0") then
      --[[
         When you perform a bootstrapping build of GCC (the default, I believe), 
         it will install GMP headers to `objdir/gmp` i.e. `build/$version/gmp`. 
         However, for GCC versions older than 13.1.0, the --disable-bootstrap 
         config option will prevent this from happening.
         
         Compiling a GCC plug-in requires having the GMP headers available, and 
         our project makefiles look for them in the GCC objdir. Ergo we'll need 
         to build GMP ourselves. We don't need to fully install it; we just need 
         its headers.
      ]]--
      if not shell:is_directory(PATHS.build .. "gmp") then
         pull_gmp_into_build_dir()
      end
      --[[
         AFAIK, this is the only dependency we have to do this for.
      ]]--
   end
end
if params.just_fix_gmp then
   pull_gmp_into_build_dir()
else
   build_and_install_gcc()
   
   console:set_window_title(fmt("Finished installing GCC %s", params.version))
   console:report_step(fmt("Finished installing GCC %s.", params.version))
   print("")
end
