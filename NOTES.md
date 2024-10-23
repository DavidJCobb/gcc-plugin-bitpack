
I'm using WSL and I am extremely *not* familiar with Linux or its ecosystem, so the notes below will be written with that in mind.

## Project setup

I chose the name `LU_GCC_INSTALLDIR` for my environment variable rather than `INSTALLDIR`, because the latter seems a tad generic, no?

### GCC

```sh
#
# make a folder to download GCC to
#
cd $HOME
mkdir gcc-dl
cd gcc-dl

#
# Download and unpack GCC source. Unpacking may take a while. The terminal will 
# accept input but otherwise be unresponsive during this time, so just wait for 
# it to show a prompt again.
#
wget http://ftp.gnu.org/gnu/gcc/gcc-11.4.0/gcc-11.4.0.tar.gz
tar xf gcc-11.4.0.tar.gz

#
# Download all prerequisites. As before, the terminal may be unresponsive, though 
# likely for not as long. This should set up GMP, MPFR, MPC, and ISL for you.
#
cd gcc-11.4.0
./contrib/download_prerequisites

cd .. # exit GCC version-specific folder
cd .. # exit gcc-dl

#
# Create a new environment variable for our install directory, so we can reference 
# it in our plug-in makefiles.
#
export LU_GCC_INSTALLDIR=$HOME/gcc/gcc-install

#
# Create what the GCC documentation calls the "objdir." We'll be building GCC from 
# this directory. Note that when building different versions, parameters, etc., 
# you'll need to run `make distclean` before a bare `make`.
#
# It's very important that the "objdir" be outside of the "srcdir," which for us 
# was /gcc-dl/whatever/.
#
mkdir gcc-build
cd gcc-build

#
# Configure the to-be-build GCC:
#
#  - prefix:           Specify where GCC will be installed to (by `make install` later)
#  - enable-languages: Choose what languages we want to be able to compile.
#  - disable-multilib: Only compile a 64-bit version of GCC.
#
# This also creates the relevant makefile(s?) for GCC in the current working directory, 
# which is why we can just run `make` straightaway afterward.
#
../gcc-dl/gcc-11.4.0/configure --prefix=$LU_GCC_INSTALLDIR --enable-languages=c,c++ --disable-multilib

#
# Build it! (Expect this to be slow, e.g. 20min.)
#
make -j$(nproc)

#
# Install it! (This may pause after showing some well-formatted messages, making it look 
# like it completed quickly... only to then immediately show more messages. Wait until 
# you see a prompt before continuing.)
#
make install

#
# Finally, run this command line to verify that it's installed properly and to the right 
# location. You'll see the computed value of the LU_GCC_INSTALLDIR environment variable 
# in the path it echoes.
#
${LU_GCC_INSTALLDIR}/bin/gcc -print-file-name=plugin
```

### GMP

If you build GCC using `download_prerequisites` to get the prerequisites, then you'll end up with an "in-tree" copy of GMP &mdash; that is: GMP is stored within the GCC source folder rather than installed system-level. This is good if you plan on experimenting with multiple GCC versions, as it means that each one is built with the specific version of GMP that it needs.

There is, ah, one problem, though. You have to tell the makefile where to find the in-tree copy of GMP. It'll be in a subfolder of the GCC "objdir," i.e. `$(HOME)/gcc-build/gmp` with our above setup.

## Potential warnings

### GCC

#### Configure

```
configure: WARNING: using in-tree isl, disabling version check
*** This configuration is not supported in the following subdirectories:
     gnattools gotools target-libada target-libhsail-rt target-libphobos target-zlib target-libbacktrace target-libgfortran target-libgo target-libffi target-libobjc target-liboffloadmic
    (Any other directories should still work fine.)
```

Shouldn't matter. The "subdirectories" listed are for the various languages that GCC can work with, and For some of their libraries.

```
/home/lu/gcc-dl/gcc-11.4.0/missing: 81: makeinfo: not found
```

Indicates that you're missing a program that bakes Texinfo files to HTML or plain-text files. Texinfo is just used for GNU documentation; we don't need it here.

## Research

[GCC itself is limited to C++11 at the newest](https://gcc.gnu.org/codingconventions.html#Portability) for portability's sake. Despite this, it's supposed to be possible to compile GCC plug-ins in C++20 per [this (fixed) bug report](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98059).


## Tips

### Opening a Linux subsystem file in Windows

Opening Windows-side files in a Windows-side program from a Linus bash script taking the file path as an argument (per [here](https://stackoverflow.com/a/72328682)):

```sh
#!/bin/bash

/mnt/c/Program\ Files/Notepad++/notepad++.exe "$(wslpath -w "$1")"
#
# the `wslpath` program converts file paths like `/mnt/c/...` to Windows paths.
#
```

Opening Linus-side files in a Windows-side program from the Windows command line is also possible (per [here](https://unix.stackexchange.com/a/600508). You may need to change the name of the top-level directory as needed to match whatever flavor of Linux you have, or alternatively, [just open the home folder in Windows Explorer directly](https://devblogs.microsoft.com/commandline/whats-new-for-wsl-in-windows-10-version-1903/).

```
Notepad++.exe \\wsl$\Ubuntu-18.04\home\user\foo.txt
```

If for whatever reason the latter doesn't work, the former means we can write a bash script to copy the to-be-viewed file to Windows `%TEMP%` and then invoke Notepad++ to open it Windows-side.

Note that on WSL1, Linux-side access to the Windows filesystem is fast, but the Linux filesystem is slow; whereas on WSL2, Linux-side access to the Windows filesystem is slow, but the Linux filesystem is fast ([source](https://news.ycombinator.com/item?id=28321568)).