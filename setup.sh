#!/bin/bash

BASEDIR=$HOME/gcc

usage() {
   echo "Usage: $0 -v <version-number>";
   echo "Version numbers should be of the form <int>.<int>.<int>."
   exit 1;
}

version=
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
      while getopts "v:" o; do
         case "${o}" in
            v)
               version=${OPTARG}
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
      parsed="$(getopt -a -l 'version:' -o 'v:' -- $@)"
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

formatting_for_error="$(printf '\x1b[0;38;2;255;64;64m')"
formatting_for_step="$(printf '\x1b[0;1m')"

# Usage: print_step "Some info: $(highlight_token foo)"
highlight_token() {
   printf "\x1b[38;2;0;255;255m$1"
}

report_step() {
   printf "\n";
   
   local blue="2;0;0;128"
   local white="2;240;240;240"
   
   # Print an "i" icon
   printf "\x1b[38;${white}m\u2590\x1b[38;${blue};48;${white}mi\x1b[0;38;${white}m\u258C\x1b[0m"
   
   printf "${formatting_for_step}%s\x1b[0m\n" "$1";
   printf "\n";
}

report_error_and_exit() {
   printf "\n";
   
   local red="2;255;64;64"
   local white="2;240;240;240"
   
   # Print an "!" icon
   printf "\x1b[38;${red}m\u2590\x1b[1;38;${white};48;${red}m!\x1b[0;38;${red}m\u258C\x1b[0m"
   
   printf "\x1b[38;${red}m%s\x1b[0m\n" "$1";
   printf "\n";
   exit 1;
}

cd $BASEDIR/dl
if [ -f "gcc-${version}.tar.gz" ]; then
   report_step "You already have the archive for GCC ${version} downloaded to $(highlight_token $HOME/gcc/dl/gcc-${version}.tar.gz)${formatting_for_step}; skipping download."
else
   report_step "Downloading GCC $(version) to $(highlight_token $HOME/gcc/dl)${formatting_for_step}..."

   wget http://ftp.gnu.org/gnu/gcc/gcc-$(version)/gcc-$(version).tar.gz
   result=$?
   if $result -eq 1; then
      report_error_and_exit "Download failed. Aborting installation process."
   elif $result -eq 3; then
      report_error_and_exit "Download failed. (File I/O issue.) Aborting installation process."
   elif $result -eq 4; then
      report_error_and_exit "Download failed. (Network failure.) Aborting installation process."
   elif $result -eq 5; then
      report_error_and_exit "Download failed. (SSL verification failure.) Aborting installation process."
   elif $result -eq 8; then
      report_error_and_exit "Download failed. (Server issued an error response; missing file?) Aborting installation process."
   elif $result -neq 0; then
      report_error_and_exit "Download failed. Aborting installation process."
   fi
fi
#
report_step "Unpacking GCC ${version} to $(highlight_token $HOME/gcc/dl/gcc-${version}/)${formatting_for_step}..."
tar xf "gcc-${version}.tar.gz" --skip-old-files
if [ $? != 0 ]; then
   report_error_and_exit "Unpacking failed. Aborting installation process."
fi

report_step "Downloading prerequisites (in-tree) for GCC ${version}..."
cd "gcc-${version}"
./contrib/download_prerequisites

cd $BASEDIR/build
mkdir ${version}
cd ..
cd install
mkdir ${version}
cd ..

report_step "Building GCC version ${version} to $(highlight_token $HOME/gcc/build/${version}/)${formatting_for_step}..."
cd build/$version/
../../dl/gcc-$version/configure --prefix=$HOME/gcc/install/$version/ --enable-languages=c,c++ --disable-multilib

make -j$(nproc)
if [ $? != 0 ]; then
   report_error_and_exit "Build failed. Aborting installation process."
fi

report_step "Installing GCC version ${version} to $(highlight_token $HOME/gcc/install/${version}/)${formatting_for_step}..."
make install
if [ $? != 0 ]; then
   report_error_and_exit "Installation failed. Aborting installation process."
fi

report_step "Finished installing GCC version ${version}."