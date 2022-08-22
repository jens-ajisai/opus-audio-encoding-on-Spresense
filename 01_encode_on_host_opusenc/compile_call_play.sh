currentDirName=`basename $PWD`
echo $currentDirName

# this is for Linux ... not tested
export LD_LIBRARY_PATH=../00_preparation/out/lib
# this is for MacOS
export DYLD_LIBRARY_PATH=../00_preparation/out/lib

# compile
/usr/bin/g++ \
    -fdiagnostics-color=always \
    -Wno-c++11-extensions \
    -I../00_preparation/out/include \
    -I../00_preparation/out/include/opus \
    -L../00_preparation/out/lib \
    -lopus \
    -lopusenc \
    -logg \
    -loggz \
    -g \
    **.cpp \
    -o \
    $currentDirName

# invoke
./$currentDirName

# play the encoded file
play ../00_preparation/test_c1_48000_s16.ogg

# play the original file
# play -t raw -e signed-integer -b 16 -c 1 -r 48000 ../00_preparation/test_c1_48000_s16.raw 