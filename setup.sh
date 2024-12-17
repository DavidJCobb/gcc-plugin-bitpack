#!/bin/bash

BASEDIR=$HOME/gcc
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
   exit 1;
}

version=
fast=0
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
      parsed="$(getopt -a -l 'version:,fast' -o 'v:f' -- $@)"
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

m4_available=$(command -v m4 > /dev/null)
pv_available=$(command -v pv > /dev/null)

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
   printf "\n";
}

report_error_and_exit() {
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
   exit 1;
}

cd $BASEDIR/dl
if [ -f "gcc-${version}.tar.gz" ]; then
   report_step "You already have the archive for GCC ${version} downloaded to <h>$HOME/gcc/dl/gcc-${version}.tar.gz</h>; skipping download."
else
   report_step "Downloading GCC ${version} to <h>$HOME/gcc/dl/</h>..."
   echo "Downloading from: http://ftp.gnu.org/gnu/gcc/gcc-${version}/gcc-${version}.tar.gz"
   echo "";

   wget "http://ftp.gnu.org/gnu/gcc/gcc-${version}/gcc-${version}.tar.gz" --no-clobber --continue
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
fi
#
report_step "Unpacking GCC ${version} to <h>$HOME/gcc/dl/gcc-${version}/</h>..."

result=0
if [ ! $pv_available ]; then
   #
   # The `pv` command is not installed. Extract files without a 
   # progress indicator.
   #
   echo ""
   
   tar xf "gcc-${version}.tar.gz" --skip-old-files --checkpoint-action=ttyout='(%d seconds elapsed): %T%*\r'
   result=$?
else
   #
   # If the `pv` command is available, then use it to show a 
   # progress bar for the extraction. In this case, `pv` is 
   # able to show progress because it's responsible for feeding 
   # the input file into `tar`, and it knows how much it's fed.
   #
   echo ""
   
   pv "gcc-${version}.tar.gz" | tar x --skip-old-files
   result=${PIPESTATUS[1]}
fi
if [ $result != 0 ]; then
   report_error_and_exit "Unpacking failed. Aborting installation process."
fi

report_step "Downloading prerequisites (in-tree) for GCC ${version}..."
echo ""

cd "gcc-${version}"
./contrib/download_prerequisites

mkdir -p $BASEDIR/build/$version
mkdir -p $BASEDIR/install/$version

report_step "Building GCC version ${version} to <h>$HOME/gcc/build/${version}/</h>...";
echo "";

cd build/$version/
#
# Start by configuring the build process. This step will create the makefiles 
# that we'll use to actually build GCC. You could think of it as building the 
# makefiles.
#
../../dl/gcc-$version/configure --prefix=$BASEDIR/install/$version/ $GCC_CONFIG
#
# Run the makefiles to build GCC.
#
make -j$(nproc)
if [ $? != 0 ]; then
   report_error_and_exit "Build failed. Aborting installation process."
fi
#
report_step "Installing GCC version ${version} to <h>$HOME/gcc/install/${version}/</h>..."
echo ""

make install
if [ $? != 0 ]; then
   report_error_and_exit "Installation failed. Aborting installation process."
fi

if [ $fast == 1 ]; then
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
   # First: We need M4, which doesn't come with WSL by default, to configure a 
   # GMP build.
   #
   if [ ! $m4_available ]; then
      report_step "Attempting to install M4 (required for GMP build)..."
      echo ""
      sudo apt-get install -y m4
      if [ $? != 0 ]; then
         report_error_and_exit "Failed to set up M4."
      fi
   fi
   #
   # Next, GMP.
   #
   if [ ! -d gmp ]; then
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
      
      report_step "Installing GMP headers to <h>$HOME/gcc/build/${version}/gmp</h>..."
      echo ""
      
      mkdir -p $BASEDIR/build/$version/gmp
      cd $BASEDIR/build/$version/gmp
      $GMPDIR/configure --prefix=$BASEDIR/build/$version/gmp
      make
      #make install # Don't need the rest of GMP; we just want the headers.
   else
      report_step "GMP headers appear to already be present in <h>$HOME/gcc/build/${version}/gmp</h>; this is unexpected, so please double-check that they're actually there."
      echo ""
   fi
   #
   # As far as I know, this is the only dependency we have to do this for.
   #
fi

report_step "Finished installing GCC version ${version}."
printf "\n";
