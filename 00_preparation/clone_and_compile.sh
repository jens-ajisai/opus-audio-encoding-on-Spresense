echo Download test file from https://opus-codec.org/static/examples/samples/speech_orig.wav and conver to raw
wget https://opus-codec.org/static/examples/samples/speech_orig.wav
dd bs=44 skip=1 if=speech_orig.wav of=test_c1_48000_s16.raw
# sox speech_orig.wav -t raw -e signed-integer -b 16 -c 1 -r 48000 test_c1_48000_s16.raw

git clone https://gitlab.xiph.org/xiph/opus.git
git clone https://github.com/gcp/libogg.git
git clone https://github.com/kfish/liboggz.git
git clone https://github.com/xiph/libopusenc.git

sed -i 's/    if ((bytes = read (fileno(oggz->file), buf, n)) == 0) {/    if ((bytes = fread (fileno(oggz->file), 1, buf, n)) == 0) {/g' liboggz/src/liboggz/oggz_io.c
sed -i 's/  return read (fileno(f), buf, n);/  return fread (fileno(f), 1, buf, n);/g' liboggz/src/examples/read-io.c

cd opus
git checkout tags/v1.3.1 -b v1.3.1
./autogen.sh
./configure --prefix=$PWD/../out
make clean
make -j
# install opus on your system globally. If you want a local installation, then use --prefix= with configure
make install

cd ../libogg
./autogen.sh && ./configure --prefix=$PWD/../out && make clean && make -j && make install

cd ../liboggz
./autogen.sh && ./configure --prefix=$PWD/../out && make clean && make -j && make install

export PKG_CONFIG_PATH=$PWD/../out/lib/pkgconfig

cd ../libopusenc
./autogen.sh && ./configure --prefix=$PWD/../out && make clean && make -j && make install
