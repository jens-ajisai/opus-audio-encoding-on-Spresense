cd ../00_preparation

source ~/spresenseenv/setup

export LDFLAGS='--specs=nosys.specs'
export CFLAGS=' -MMD -std=gnu11 -fno-builtin -mabi=aapcs -Wall -Os -fno-strict-aliasing -fno-strength-reduce -fomit-frame-pointer -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -pipe -s -gdwarf-3 -ffunction-sections -fdata-sections'
export CXXFLAGS=' -MMD -std=gnu++11 -c -fno-builtin -mabi=aapcs -Wall -Os -fno-strict-aliasing -fno-strength-reduce -fomit-frame-pointer -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -pipe -ffunction-sections -fno-exceptions -fno-rtti -s -ffunction-sections -fdata-sections -fpermissive'
export CPPFLAGS=' -D_FORTIFY_SOURCE=0 -Wa,-mimplicit-it=thumb'

sed -i 's/SUBDIRS = liboggz tools tests examples/SUBDIRS = liboggz tests examples/g' liboggz/src/Makefile.in 

cd opus
./configure --host=arm-none-eabi --prefix=$PWD/../outSpresense --disable-rtcd --enable-fixed-point --disable-float-api --enable-float-approx --disable-doc --disable-extra-programs
sed -i 's/#define OPUS_ARM_MAY_HAVE_NEON_INTR 1//g' config.h 
make clean && make -j && make install

cd ../libogg
./configure --host=arm-none-eabi --prefix=$PWD/../outSpresense
make clean && make -j && make install

export PKG_CONFIG_PATH=$PWD/../outSpresense/lib/pkgconfig

cd ../liboggz
./configure --host=arm-none-eabi --prefix=$PWD/../outSpresense --disable-examples
make clean && make -j && make install

# only compiles if the float API is enabled at opus build.
# cd ../libopusenc
# ./configure --host=arm-none-eabi  --prefix=$PWD/../outSpresense
# make clean && make -j && make install
