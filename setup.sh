#!/bin/bash

BASEDIR=$HOME/gcc
mkdir -p $BASEDIR
mkdir -p $BASEDIR/dl
mkdir -p $BASEDIR/build
mkdir -p $BASEDIR/install
#
# The srcdir is $BASEDIR/dl/gcc-$version and the objdir is $BASEDIR/build/$version.
#

#
# Options for the GCC build.
#
#    --enable-languages
#       What languages we wish to be able to compile.
#
#    --disable-multilib
#       Only build an x64 GCC that is, itself, only able to build software 
#       for x64. (If you're in the future, sub out "x64" for whatever CPU 
#       architecture you use.)
#
GCC_CONFIG="--enable-languages=c,c++ --disable-multilib"

usage() {
   echo "Usage:";
   echo " setup.sh [options]"
   echo ""
   echo "Downloads and installs a given GCC version using a hardcoded folder hierarchy that "
   echo "permits multiple versions to live side-by-side. The plug-in project makefiles are "
   echo "set up to rely on the same folder hierarchy."
   echo ""
   echo "Required options::"
   echo " -v, --version <a>.<b>.<c>       desired GCC version"
   echo ""
   echo "Additional options:"
   echo " -f, --fast                      fast build (w/o localization, bootstrapping, etc.)"
   echo " --erase-prerequisites           clean prerequisites first (e.g. to fix corrupt downloads)"
   echo " --just-fix-gmp                  get GMP in the build directory and do nothing else"
   echo " --threads <positive-integer>    override thread count (default is nproc i.e. all cores)"
   echo "                                 (useful for building multiple GCCs at a time, since not "
   echo "                                 all build steps seem to parallelize well)"
   exit 1;
}

version=
fast=0
erase_prerequisites=0
just_fix_gmp=0
threads=$(nproc)

is_integer() {
   local regex='^-?[0-9]+$'
   #
   # NOTE: inlining the regex into the comparison below, even with single-quotes, 
   #       changes the semantics and breaks the expression.
   #
   if [[ $1 =~ $regex ]]; then
      return 0
   fi
   return 1
}

extract_args() {
   #
   # Check for GNU's variant on getopt, which properly supports long option 
   # names and spaces within argument values. If it's not present, then fall 
   # back to Bash's getops (note the "S"), which doesn't support long option 
   # names but is less broken than most system-level getopt programs.
   #
   local vercheck=$(getopt -T)
   local parsed=
   if (( $? != 4)) && [[ -n $vercheck ]]; then
      while getopts "v:f" o; do
         case "${o}" in
            v)
               version=${OPTARG}
               ;;
            f)
               fast=1
               ;;
            *)
               usage
               ;;
         esac
      done
      shift $((OPTIND-1))
   else
      #
      # The syntax for -l, the long option name list, is a comma-separated 
      # list of option names. When a name is suffixed with a colon, the 
      # option requires an argument.
      #
      # The syntax for -o, the short option name list, is similar except 
      # for the lack of separators and the fact that all names are only 
      # one character long.
      #
      parsed="$(getopt -a -l 'version:,fast,erase-prerequisites,just-fix-gmp,threads:' -o 'v:f' -- $@)"
      if [ $? != "0" ]; then
         echo "bad"
         usage
      fi
      eval set -- "$parsed" # overwrite current function/command params
      unset parsed
      while true
      do
         case $1 in
            "-v"|"--version")
               version="$2"
               shift 2 # consume the key and the value
               ;;
            "-f"|"--fast")
               fast=1
               shift
               ;;
            "--erase-prerequisites")
               erase_prerequisites=1
               shift
               ;;
            "--just-fix-gmp")
               just_fix_gmp=1
               shift
               ;;
            "--threads")
               threads=$2
               shift 2 # consume the key and the value
               
               if ! is_integer "$threads"; then
                  echo "number of threads, if specified, must be a positive integer (saw ${threads} which is not an integer)"
                  usage
               fi
               if (( $threads <= 0 )); then
                  echo "number of threads, if specified, must be a positive integer (saw ${threads} which is zero or negative)"
                  usage
               fi
               
               ;;
            # Boilerplate:
            --) # leftover: the argument separator we gave to `getopt`
               shift
               break
               ;;
            *) # unrecognized parameter
               echo "unrecognized parameter $1"
               usage
               ;;
         esac
      done
   fi
}; extract_args "$@";

# Check if the `version` variable is an empty string.
if [ -z "${version}" ] ; then
   usage
fi

bzip2_available=$(command -v bzip2 > /dev/null; echo $?)
m4_available=$(command -v m4 > /dev/null; echo $?)
pv_available=$(command -v pv > /dev/null; echo $?)

setup_gcc_needed_programs() {
   local bzip2_available=$(command -v bzip2 > /dev/null; echo $?)
   local make_available==$(command -v make > /dev/null; echo $?)
   if [[ $bzip2_available != 0 ]] || [[ $make_available != 0 ]]; then
      local dependencies_list=()
      if [[ $bzip2_available != 0 ]]; then
         dependencies_list+=bzip
      fi
      
      set_window_title "Attempting to install needed programs for GCC..."
      report_step "Attempting to install needed programs for GCC..."
      echo ""
      
      # Update the list of available packages to install. Doesn't 
      # actually install updates; just updates our list of the 
      # available updates. Required, or installing `build-essential` 
      # will fail on a fresh Ubuntu 24.04.1 LTS.
      #
      # Fun fact: `apt update` and `apt-get update` aren't the same.
      sudo apt update
      sudo apt-get update --fix-missing
      
      sudo apt-get install -y $dependencies_list --fix-missing
      if [ $? != 0 ]; then
         report_error_and_exit "Failed to set up needed programs."
      fi
      
      if [[ $make_available != 0 ]]; then
         sudo apt-get install -y build-essential --fix-missing
         if [ $? != 0 ]; then
            report_error_and_exit "Failed to set up needed build-essential tools."
         fi
      fi
   fi
}
setup_m4() {
   if [ $m4_available != 0 ]; then
      set_window_title "Attempting to install M4 (required for GMP build)..."
      report_step "Attempting to install M4 (required for GMP build)..."
      echo ""
      sudo apt-get install -y m4
      if [ $? != 0 ]; then
         report_error_and_exit "Failed to set up M4."
      fi
   fi
}

if [ $fast == 1 ]; then
   #
   # --disable-nls
   #    Don't install localization files.
   #
   # --disable-dependency-tracking
   #    Don't track and cache dependency information that would be useless 
   #    for one-off builds of a program that I don't intend to modify.
   #
   # --disable-bootstrap
   #    Use a simpler build process, because we don't care whether the GCC 
   #    we're building here can build other GCCs.
   #
   # On my machine (WSL1 on an SSD), this cuts GCC's build time down from 
   # 45 minutes to about 11 minutes. This doesn't include the time it takes 
   # for us to build GMP into the folder our plug-in project expects it to 
   # be in, though.
   #
   GCC_CONFIG="${GCC_CONFIG} --disable-nls --disable-dependency-tracking --disable-bootstrap"
fi

formatting_for_highlight="$(printf '\x1b[38;2;0;255;255m')"
formatting_for_error="$(printf '\x1b[0;38;2;255;64;64m')"
formatting_for_step="$(printf '\x1b[0;1m')"

report_step() {
   printf "\n";
   
   local blue="2;0;0;128"
   local white="2;240;240;240"
   
   # Print an "i" icon
   printf "\x1b[38;${white}m\u2590\x1b[38;${blue};48;${white}mi\x1b[0;38;${white}m\u258C\x1b[0m"
   
   local text="$1"
   text="${text//<h>/$formatting_for_highlight}"
   text="${text//<\/h>/$formatting_for_step}"
   
   printf "${formatting_for_step}%s\x1b[0m\n" "${text}";
}
report_error() {
   printf "\n";
   
   local red="2;255;64;64"
   local white="2;240;240;240"
   
   # Print an "!" icon
   printf "\x1b[38;${red}m\u2590\x1b[1;38;${white};48;${red}m!\x1b[0;38;${red}m\u258C\x1b[0m"
   
   local text="$1"
   text="${text//<h>/$formatting_for_highlight}"
   text="${text//<\/h>/$formatting_for_step}"
   
   printf "\x1b[38;${red}m%s\x1b[0m\n" "${text}";
   printf "\n";
}
report_error_and_exit() {
   report_error "$1";
   exit 1;
}

set_window_title() {
   #
   # Testing shows that this works when running Bash via WSL via Powershell 
   # in Windows 10. No clue about other terminals, nor about how to check 
   # whether this is supported.
   #
   # Window titles will be reset when control returns to the user (read: 
   # when our script exits).
   #
   printf "\e]0;%s\e\x5C" "$1"
}

cd $BASEDIR/dl

deal_with_archive() {
   download_archive() {
      set_window_title "Downloading GCC ${version}..."
      report_step "Downloading GCC ${version} to <h>$HOME/gcc/dl/</h>..."
      echo "Downloading from: http://ftp.gnu.org/gnu/gcc/gcc-${version}/gcc-${version}.tar.gz"
      echo "";

      wget "http://ftp.gnu.org/gnu/gcc/gcc-${version}/gcc-${version}.tar.gz" --continue
      result=$?
      if [ $result == 1 ]; then
         report_error_and_exit "Download failed. Aborting installation process."
      elif [ $result == 3 ]; then
         report_error_and_exit "Download failed. (File I/O issue.) Aborting installation process."
      elif [ $result == 4 ]; then
         report_error_and_exit "Download failed. (Network failure.) Aborting installation process."
      elif [ $result == 5 ]; then
         report_error_and_exit "Download failed. (SSL verification failure.) Aborting installation process."
      elif [ $result == 8 ]; then
         report_error_and_exit "Download failed. (Server issued an error response; missing file?) Aborting installation process."
      elif [ $result != 0 ]; then
         report_error_and_exit "Download failed. Aborting installation process."
      fi
   }
   unpack_archive() {
      local extra_params=
      if [ -z "$1" ]; then
         extra_params="${extra_params} --skip-old-files"
      fi

      set_window_title "Unpacking GCC ${version}..."
      report_step "Unpacking GCC ${version} to <h>$HOME/gcc/dl/gcc-${version}/</h>..."
      echo ""
      
      result=0
      if [[ $pv_available != 0 ]]; then
         #
         # The `pv` command is not installed. Extract files without a 
         # progress indicator.
         #
         tar xf "gcc-${version}.tar.gz" $extra_params --checkpoint-action=ttyout='(%d seconds elapsed): %T%*\r'
         result=$?
      else
         #
         # If the `pv` command is available, then use it to show a 
         # progress bar for the extraction. In this case, `pv` is 
         # able to show progress because it's responsible for feeding 
         # the input file into `tar`, and it knows how much it's fed.
         #
         pv "gcc-${version}.tar.gz" | tar x --skip-old-files
         result=${PIPESTATUS[1]}
      fi
      return $result;
   }

   local already_have_archive=0
   if [ -f "gcc-${version}.tar.gz" ]; then
      report_step "You already have the archive for GCC ${version} downloaded to <h>$HOME/gcc/dl/gcc-${version}.tar.gz</h>; skipping download."
      already_have_archive=1
   else
      download_archive;
   fi

   unpack_archive;
   if (( $? != 0 )); then
      if (( $already_have_archive != 0 )); then
         #
         # The user already had the archive file, so we didn't bother 
         # downloading it. If it was an interrupted download, though, 
         # then that will have been a mistake.
         #
         report_error "Unpacking failed. Perhaps the download was interrupted? Re-downloading..."
         
         download_archive;
         unpack_archive 'dont_skip_old_files';
         if (( $? != 0 )); then
            report_error_and_exit "Unpacking failed again. Aborting installation process."
         fi
      else
         report_error_and_exit "Unpacking failed. Aborting installation process."
      fi
   fi
}
if [ $just_fix_gmp == 0 ]; then
   deal_with_archive;
fi

deal_with_prereqs() {
   cd "${BASEDIR}/dl/gcc-${version}"
   if [ $erase_prerequisites != 0 ]; then
      set_window_title "Cleaning out prerequisites (in-tree) for GCC ${version}..."
      report_step "Cleaning out prerequisites (in-tree) for GCC ${version}..."
      echo ""
      
      find . -name 'gettext-*.tar.*' -exec rm {} \;
      find . -name 'gmp-*.tar.*' -exec rm {} \;
      find . -name 'isl-*.tar.*' -exec rm {} \;
      find . -name 'mpc-*.tar.*' -exec rm {} \;
      find . -name 'mpfr-*.tar.*' -exec rm {} \;
   fi
   #
   set_window_title "Downloading prerequisites (in-tree) for GCC ${version}..."
   report_step "Downloading prerequisites (in-tree) for GCC ${version}..."
   echo ""
   #
   setup_gcc_needed_programs
   ./contrib/download_prerequisites
   if (( $? != 0 )); then
      report_error_and_exit "Failed to download prerequisites. Aborting installation process. Recommend retrying with --erase-prerequisites to clear any corrupt downloads."
   fi
}
if [ $just_fix_gmp == 0 ]; then
   deal_with_prereqs;
fi

pull_gmp_into_build_dir() {
   #
   # Building GMP via its configure script requires M4.
   #
   setup_m4;
   
   find_gmp_directory() {
      cd $BASEDIR/dl/gcc-$version/
      local pattern="gmp-"
      local current=
      local result=
      for current in "${pattern}"*; do
          [ -d "${current}" ] && result="${current}" && break
      done
      echo "${result}"
   }
   GMPDIR="${BASEDIR}/dl/gcc-${version}/$(find_gmp_directory)"
   
   set_window_title "Installing GMP headers..."
   report_step "Installing GMP headers to <h>$HOME/gcc/build/${version}/gmp</h>..."
   echo ""
   
   mkdir -p $BASEDIR/build/$version/gmp
   cd $BASEDIR/build/$version/gmp
   $GMPDIR/configure --prefix=$BASEDIR/build/$version/gmp
   make -j$threads
   #make install # Don't need the rest of GMP; we just want the headers.
}

build_and_install_gcc() {
   mkdir -p $BASEDIR/build/$version
   mkdir -p $BASEDIR/install/$version

   set_window_title "Building GCC ${version}..."
   report_step "Building GCC ${version} to <h>$HOME/gcc/build/${version}/</h>...";
   echo "";

   cd $BASEDIR/build/$version/
   #
   # Start by configuring the build process. This step will create the makefiles 
   # that we'll use to actually build GCC. You could think of it as building the 
   # makefiles.
   #
   ../../dl/gcc-$version/configure --prefix=$BASEDIR/install/$version/ $GCC_CONFIG
   #
   # Run the makefiles to build GCC.
   #
   make -j$threads
   if [ $? != 0 ]; then
      report_error_and_exit "Build failed. Aborting installation process."
   fi
   #
   set_window_title "Installing GCC ${version}..."
   report_step "Installing GCC version ${version} to <h>$HOME/gcc/install/${version}/</h>..."
   echo ""

   #
   # DO NOT parallelize `make install`; it doesn't appear to be designed for 
   # that, and failures have been reported, e.g.
   # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=42980
   #
   make install
   if [ $? != 0 ]; then
      report_error_and_exit "Installation failed. Aborting installation process."
   fi
   
   # Returns a bash bool (zero is true, nonzero is false)
   directory_actually_exists_make_goddamned_sure() {
      if [ -d "$1" ]; then
         #
         # -d has in my experience been unreliable and reported false-positives 
         # w.r.t. directories existing. Let's `cd` into the directory and make 
         # sure it actually exists.
         #
         local prior=$PWD
         cd "$1"
         local result=$?
         cd $prior
         return $result
      fi
      return 1
   }
   
   if [[ $fast == 1 ]]; then
      #
      # When you perform a bootstrapping build of GCC (the default, I believe), 
      # it will install GMP headers to `objdir/gmp`, i.e. `build/$version/gmp`. 
      # However, the --disable-bootstrap config option will prevent this from 
      # happening.
      #
      # Compiling a GCC plug-in requires having the GMP headers available, and 
      # my project makefiles look for them in the GCC objdir. Ergo we'll need 
      # to build GMP ourselves. We don't need to fully install it; we just need 
      # its headers.
      #
      # TODO: GCC 13.1.0 puts GMP in that folder. Is its absence limited to 
      #       older GCC versions?
      #
      cd "${BASEDIR}/build/${version}"
      if directory_actually_exists_make_goddamned_sure "${BASEDIR}/build/${version}/gmp"; then
         report_step "GMP headers appear to already be present in <h>$HOME/gcc/build/${version}/gmp</h>/. This is unexpected, so please double-check that they're actually there."
         echo "If the headers aren't there, run this script with --just-fix-gmp."
         echo ""
      else
         pull_gmp_into_build_dir;
      fi
      #
      # As far as I know, this is the only dependency we have to d o this for.
      # (The space in that word is intentional. Somehow, Bash sometimes still 
      # parses keywords inside of these line comments. Incredible.)
      #
   fi
}
if $just_fix_gmp; then
   build_and_install_gcc;
fi

if [[ $just_fix_gmp != 0 ]]; then
   pull_gmp_into_build_dir;
fi

set_window_title "Finished installing GCC ${version}"
report_step "Finished installing GCC version ${version}."
printf "\n";
