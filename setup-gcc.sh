
#
# Create a new environment variable for our install directory, so we can reference 
# it in our plug-in makefiles.
#
# NOTE: Apparently, bash scripts can't define environment variables. Something 
# about it being scoped to the current script/shell session? Define this top-level 
# by running the command yourself.
#
export LU_GCC_INSTALLDIR=$HOME/gcc/gcc-install

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