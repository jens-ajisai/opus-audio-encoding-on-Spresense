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
play ../00_preparation/test_c1_48000_s16.ogg

# this did not work ..
# ffmpeg -loglevel debug -y -acodec libopus -ac 1 -ar 48000 -i ../00_preparation/test_c1_48000_s16.ogg -b:a 128k -ar 48000 -f mp3 test_c1_48000_s16.mp3