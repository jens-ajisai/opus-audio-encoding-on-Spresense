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

# decode the encoded file using the opus_demo
../00_preparation/opus/opus_demo  -d 48000 1 ../00_preparation/test_c1_48000_s16.opus ../00_preparation/test_c1_48000_s16_decoded.raw
play -t raw -e signed-integer -b 16 -c 1 -r 48000 ../00_preparation/test_c1_48000_s16_decoded.raw

# just for reference. The encode command for opus_demo. The file is much larger though than the one I get.
# ./opus_demo -e voip 48000 1 768000 -framesize 10 -complexity 0 /Users/jens/Documents/test_c1_48000_s16.wav test_c1_48000_s16.opus