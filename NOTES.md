
I'm using WSL and I am extremely *not* familiar with Linux or its ecosystem, so the notes below will be written with that in mind.

## Project setup

I chose the name `LU_GCC_INSTALLDIR` for my environment variable rather than `INSTALLDIR`, because the latter seems a tad generic, no?

I'm currently targeting GCC 11.4, so [this reference of C++ standards support by GCC version](https://gcc.gnu.org/projects/cxx-status.html) should be useful.

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

## Potential warnings and errors

### Building GCC

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

### Building plug-ins

#### Link errors when running a plug-in

A typical example:

```
cc1: error: cannot load plugin ./function-manip.so: ./function-manip.so: undefined symbol: _Z13cp_type_qualsPK9tree_node
```

These errors are generally unintuitive because they'll be caused by the underlying functions that several GCC macros use. For example, the above was caused by `DECL_CONST_MEMFUNC_P`, which uses the `cp_type_quals` function (from `/cp/cp-tree.h`) to check whether a member function declaration is a const member function.

The problem *usually* results from calling C++-related functions while your plug-in is being used to compile C code. Try to avoid that. Note that some functions may vary between C and C++, and the plugin headers may only include the C++ version (`comptypes`, the underlying function for `same_type_p`, is one example).

If that's *not* your issue, then it is technically possible for something to go wrong while building GCC, such that the built executable simply doesn't export all of the functions that it's supposed to. It's difficult to actually find potential explanations for that, though, in part because every conceivable combination of search terms you could ever use will be polluted by people who are having problems *using* GCC, and in part because this entire class of problem falls into the category of "weirdo nonsense." To illustrate the latter point: back in 2022, [using Homebrew to install GCC on an x86/x64 machine running MacOS](https://github.com/Homebrew/homebrew-core/issues/106394) could cause GCC to be missing exports due to the MacOS `strip` tool being overenthusiastic.

You can use the `nm` utility to dump a list of all of a program or object file's exports. I recommend dumping to a file rather than a console, since you can't copy that much text out of the WSL terminal when it's running in Powershell.

## Research

[GCC itself is limited to C++11 at the newest](https://gcc.gnu.org/codingconventions.html#Portability) for portability's sake. Despite this, it's supposed to be possible to compile GCC plug-ins in C++20 per [this (fixed) bug report](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98059).

* [DECL](https://gcc.gnu.org/onlinedocs/gcc-3.3.6/gccint/Declarations.html)-type tree data
* [TYPE](https://gcc.gnu.org/onlinedocs/gcc-3.3.6/gccint/Types.html)-type tree data
* [Accessing attributes on a DECL or TYPE](https://gcc.gnu.org/onlinedocs/gcc-3.3.6/gccint/Attributes.html)
* [TREE_LIST and TREE_VEC](https://gcc.gnu.org/onlinedocs/gcc-3.3.6/gccint/Containers.html) info

Generating code and control flow structures in GENERIC:

* [`PREINCREMENT_EXPR`](https://github.com/giuseppe/gccbrainfuck/blob/60f0e00b512fb0466b23e5a86f3c8ffe50ae665d/brainfuck-lang.c#L176) and integer constants
* Addressof operator: `build_unary_op(input_location, ADDR_EXPR, decl, 0)`
* Converting a FOR-loop from the C-specific `FOR_STMT` to GENERIC
  * [`genericize_for_stmt`](https://codebrowser.dev/gcc/gcc/c-family/c-gimplify.cc.html#_ZL19genericize_for_stmtPP9tree_nodePiPvPFS0_S1_S2_S3_EPFS0_S1_S2_S5_S3_P8hash_setIS0_Lb0E19default_hash_traitsIS0_EEE)
    * Takes a function pointer argument that gets applied to all instruction trees within the for-loop. This is used to recursively transform nested for-loops all in one go. It can be safely ignored for our purposes.
  * [`genericize_c_loop`](https://codebrowser.dev/gcc/gcc/c-family/c-gimplify.cc.html#_ZL17genericize_c_loopPP9tree_nodejS0_S0_S0_bPiPvPFS0_S1_S2_S3_EPFS0_S1_S2_S5_S3_P8hash_setIS0_Lb0E19default_hash_traitsIS0_EEE)

GCC's own parsing:

* Function definitions
  * `start_function` (`c/c-decl.cc`) synthesizes `DECLSPECS`, a `DECLARATOR`, and an attribute list into a `FUNCTION_DECL`, fires off the `PLUGIN_START_PARSE_FUNCTION` callback for that decl, and then calls `start_preparsed_function` to "create the `FUNCTION_DECL`."

## Tips

### The makefile and the build process

To build any of the plug-ins, navigate to its folder and run `make`. To test it on a sample C file, run `make test`. If the build fails, if it seems like your code changes aren't doing anything, or if you start getting weird segfaults or memory corruption issues without clear cause, then delete all of the build's output files in order to run a clean build, and see if that helps you.

Make is a build automation tool that was created half a century ago, designed to detect when a file or its dependencies change and automatically rebuild that file. Despite being built for that exact purpose and AFAIK originally for the C language, make has no means of understanding `#include`s, and so will fail to properly recompile a file if *that* file isn't modified but another C file that it depends on (by way of including a corresponding H) *is* modified. It's [possible to use GCC and specialized make rules to work around this deficiency](https://make.mad-scientist.net/papers/advanced-auto-dependency-generation/), but I haven't been able to get them to work: the exact problems that that article claims to solve still occur, with no way to meaningfully debug them thanks to make's vague error reporting.

The most reliable way to build anything is to just completely delete all build output (e.g. `.o` files) and do a clean rebuild from scratch, almost entirely defeating the purpose of using make in the first place. There are tools that layer on top of makefiles to deal with dependencies (supposedly) more reliably, like Automake, but that one's a third of a century old, and as of this writing I literally do not have the time to install, test, and inevitably struggle with yet more ancient utilities and the litany of dependencies that a lot of them have. It's faster to do things the low-tech way and just manually clean the build.

Useful things to know for makefiles:

* [String manipulation functions](https://www.gnu.org/software/make/manual/html_node/Text-Functions.html) e.g. `patsubst`
* [Path manipulation functions](https://www.gnu.org/software/make/manual/html_node/File-Name-Functions.html). In the context of make, the precise operational definition of a "file name" is a file *path*.
* [Automatic variables](https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html) e.g. `$@` and friends


### Opening a Linux subsystem file in Windows

Opening Windows-side files in a Windows-side program from a Linus bash script taking the file path as an argument (per [here](https://stackoverflow.com/a/72328682)):

```sh
#!/bin/bash

/mnt/c/Program\ Files/Notepad++/notepad++.exe "$(wslpath -w "$1")"
#
# the `wslpath` program converts file paths like `/mnt/c/...` to Windows paths.
#
```

Opening Linux-side files in a Windows-side program from the Windows command line is also possible (per [here](https://unix.stackexchange.com/a/600508). You may need to change the name of the top-level directory as needed to match whatever flavor of Linux you have, or alternatively, [just open the home folder in Windows Explorer directly](https://devblogs.microsoft.com/commandline/whats-new-for-wsl-in-windows-10-version-1903/).

```
Notepad++.exe \\wsl$\Ubuntu-18.04\home\user\foo.txt
```

Windows Explorer will also show a "Linux" category at the bottom of its sidebar while WSL is running, allowing fast and easy access to Linux-side files without having to `cd` back to `$HOME` in the Linux shell. Neat!

Note that on WSL1, Linux-side access to the Windows filesystem is fast, but the Linux filesystem is slow; whereas on WSL2, Linux-side access to the Windows filesystem is slow, but the Linux filesystem is fast ([source](https://news.ycombinator.com/item?id=28321568)).