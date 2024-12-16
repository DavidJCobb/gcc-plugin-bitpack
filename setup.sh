#!/bin/bash

usage() {
   echo "Usage: $0 -v=<version-number>";
   echo "Version numbers should be of the form <int>.<int>.<int>."
   exit 1;
}

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

while getopts ":v" o; do
   case "${o}" in
      v)
         version=${OPTARG}
         ;;
   esac
done
shift $((OPTIND-1))

# Check if the `version` variable is an empty string.
if [ -z "${version}" ] ; then
   usage
fi

report_step "Downloading GCC ${version} to $(highlight_token $HOME/gcc/dl)${formatting_for_step}..."

cd $HOME/gcc/dl
wget http://ftp.gnu.org/gnu/gcc/gcc-${version}/gcc-${version}.tar.gz
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
#
report_step "Unpacking GCC ${version}... to $(highlight_token $HOME/gcc/dl/gcc-$version/)${formatting_for_step}..."
tar xf gcc-${version}.tar.gz

cd ..
cd build
mkdir ${version}
cd ..
cd install
mkdir ${version}
cd ..

report_step "Building GCC version ${version} to $(highlight_token $HOME/gcc/build/$version/)${formatting_for_step}..."
cd build/${version}/
../../dl/gcc-${version}/configure --prefix=$HOME/gcc/install/${version}/ --enable-languages=c,c++ --disable-multilib

make -j$(nproc)

report_step "Installing GCC version ${version} to $(highlight_token $HOME/gcc/install/$version/)${formatting_for_step}..."
make install

report_step "Finished installing GCC version ${version}."