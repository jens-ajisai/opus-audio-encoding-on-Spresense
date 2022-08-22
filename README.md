# Opus Audio encoding and streaming on Spresense
## Summary of the Content
We will learn about
* using libopus library to encode Opus audio
* how to compile an external library for Spresense and integrate it into a (Arduino) project
* wrapping the data into a container format like ogg
* encode data from a file and live data recorded on Spresense
* stream the audio via serial and convert it into UDP with a python script
* receive the stream and convert it to any other format using VLC and FFMpeg tool

We will split up into the following steps as this is a quite complex task. We need to make sure that each part is working to avoid tons of issues at the same time, that are hard to debug.
1. Encode a pre-recorded test file on a host machine 
	* (a) using libopusenc as this does everything incl ogg format in a simple single command.
	* (b) using libopus using the opus_demo format. (and test by decoding and listening)
2. Package the encoded data with the ogg format on a host machine. (and test by decoding and listening)
3. Compile the libraries for Spresense and try the libopusenc approach
4. Port the libopus and libogg(z) approach to Spresense.
5. Stream the data via serial and play the stream with VLC
6. Switch the test recording with live recording
7. Move the encoder work onto a Subcore using ASMP
8. Create a DSP binary that can be used with the Audio framework (not finished)

## Motivation
* Spresense Audio Library for Arduino includes an option to use Opus audio codec and the SDK for NuttX is also prepared for configuring and using Opus. However a DSP binary for Opus codec is not provided.
* For streaming audio, the data volume is a crucial factor. LTE-M is limited in speed and for most providers fees are based on data volume at a premium price (compared to regular LTE rates)

## Preparation
We use the following libraries
* libopus
	* Opus is a totally open, royalty-free, highly versatile audio codec. 
	* Link [GitHub - xiph/opus: Modern audio compression for the internet.](https://github.com/xiph/opus)
	* Website [Opus Codec](https://opus-codec.org)
	* Documentation [Opus: Opus](https://opus-codec.org/docs/opus_api-1.3.1/index.html)
	* Specification [RFC 6716: Definition of the Opus Audio Codec](https://www.rfc-editor.org/rfc/rfc6716)
* libogg
	* Ogg is a multimedia container format, and the native file and stream format for the Xiph.org multimedia codecs. Opus files are usually inside the Ogg container format. See [RFC 7845 - Ogg Encapsulation for the Opus Audio Codec](https://datatracker.ietf.org/doc/html/rfc7845)
	* Link [GitHub - gcp/libogg: Ogg container format libraries](https://github.com/gcp/libogg)
	* Website [Xiph.org: Ogg](https://xiph.org/ogg/)
	* Documentation [libogg - Documentation](https://xiph.org/ogg/doc/libogg/)
	* Specification [RFC 3533:  The Ogg Encapsulation Format Version 0](https://www.rfc-editor.org/rfc/rfc3533)
* liboggz
	* Oggz comprises liboggz and the tool oggz, which provides commands to inspect, edit and validate Ogg files. 
	* Link [GitHub - kfish/liboggz: A library and tools for working with Ogg encapsulation, with support for seeking, chopping and validation.](https://github.com/kfish/liboggz)
	* Website [Xiph.org: Oggz](https://xiph.org/oggz/)
	* Documentation [liboggz: Main Page](https://xiph.org/oggz/doc/)
* libopusenc
	* The libopusenc libraries provide a high-level API for encoding .opus files.
	* Link [GitHub - xiph/libopusenc: Library for encoding .opus audio files and live streams.](https://github.com/xiph/libopusenc)
	* Website [Opus Codec](https://opus-codec.org)
	* Documentation [libopusenc: Main Page](https://opus-codec.org/docs/libopusenc_api-0.2/index.html)

For testing we also use
* FFmpeg [FFmpeg](https://ffmpeg.org)
* Audacity [Audacity ® | Free, open source, cross-platform audio software for multi-track recording and editing.](https://www.audacityteam.org) or Sound eXchange [SoX - Sound eXchange | HomePage](http://sox.sourceforge.net)
* Python ([Welcome to Python.org](https://www.python.org)) with additional libraries
	* pyserial-asyncio [Welcome to pySerial-asyncio’s documentation — pySerial-asyncio 0.6 documentation](https://pyserial-asyncio.readthedocs.io/en/latest/)

A development toolchain
* (depending on your host OS) for example autoconf, make, gcc etc.

##  Encode a pre-recorded test file on a host machine
### Prepare test recording
We prepare a test recording for example with Audacity and save the file as raw audio data without header as 1 channel 16bit signed integer. Alternatively we can use [SoX - Sound eXchange | HomePage](http://sox.sourceforge.net) with the command line. 
`rec -e signed-integer -b 16 -c 1 -r 48000 test_c1_48000_s16.raw`
The prepared scripts download one of the samples from [Opus Codec](https://opus-codec.org) and convert it to headerless raw format.

### Compile the libraries
The commands are summarized in 00_preparation/clone_and_compile.sh
We clone the projects and build them for the host machine
```
git clone https://gitlab.xiph.org/xiph/opus.git
git clone https://github.com/gcp/libogg.git
git clone https://github.com/kfish/liboggz.git
git clone https://github.com/xiph/libopusenc.git

cd opus
git checkout tags/v1.3.1 -b v1.3.1
./autogen.sh 
./configure --prefix=$PWD/../out
make -j
make install

cd ../libogg
./autogen.sh && ./configure --prefix=$PWD/../out && make -j && make install

export PKG_CONFIG_PATH=$PWD/../out/lib/pkgconfig

cd ../liboggz
./autogen.sh && ./configure --prefix=$PWD/../out && make -j && make install

cd ../libopusenc
./autogen.sh && ./configure --prefix=$PWD/../out && make -j && make install

```

I got a compile error for liboggz 
```
oggz_io.c:58:18: error: implicit declaration of function 'read' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
    if ((bytes = read (fileno(oggz->file), buf, n)) == 0) {
```
therefore I change this line at liboggz/src/liboggz/oggz_io.c to
`    if ((bytes = fread (fileno(oggz->file), 1, buf, n)) == 0) {`

same for liboggz/src/examples/read-io.c
```
read-io.c:76:10: error: implicit declaration of function 'read' is invalid in C99 [-Werror,-Wimplicit-function-declaration]
  return read (fileno(f), buf, n);
```
to
`  return fread (fileno(f), 1, buf, n);`

The scripts use sed to achieve this.
```
sed -i 's/    if ((bytes = read (fileno(oggz->file), buf, n)) == 0) {/    if ((bytes = fread (fileno(oggz->file), 1, buf, n)) == 0) {/g' liboggz/src/liboggz/oggz_io.c
sed -i 's/  return read (fileno(f), buf, n);/  return fread (fileno(f), 1, buf, n);/g' liboggz/src/examples/read-io.c
```

### Compile the libopusenc based test program execute and play
The commands are summarized in 01_encode_on_host_opusenc/compile.sh

There is an example for libopusenc inside libopusenc/examples/opusenc_example.c that can be used out of the box. I mainly copied that code and removed the command line argument parsing (That will be necessary on Spresense later). See 01_encode_on_host_opusenc/encoderPoc_opusenc.cpp.

Open the file with vlc or call
`play ../00_preparation/test_c1_48000_s16.ogg`

The quality is quite bad ... not sure why. It will be much better in the next steps.

### Compile the opus based test program execute and play (opus_demo format)
The commands are summarized in 02_encode_on_host_opus_opus_demo/compile.sh

I started with the opus_demo.c, which it quite large. Later I found out that there is a simple sample under opus/doc/trivial_example.c.
The take aways are
* The default configuration is very good. See [Opus Recommended Settings - XiphWiki](https://wiki.xiph.org/Opus_Recommended_Settings). Therefore we do not set any configuration. The only item we set later will be the complexity. So for now applyConfiguration() does not do anything.
* I used OPUS_APPLICATION_VOIP for my use case.
* It is required to provide a number of frames that match the frame sizes opus accepts. See include/opus/opus_defines.h for OPUS_FRAMESIZE_*. We use 480 frames which corresponds to 10ms (The reocommended frame size is 20ms). The data provided must be of size frame * number of channels * sizeof(uint16_t). We have 1 channel and 16bit values. So it will be 960.
* The opus_demo uses a custom container format, which is <4 bytes data lenght> <4 bytes final range> <n bytes data>. Byte order for data lenght and final range is reversed. Therefore the int_to_char() function as in the opus_demo.c.
* If you are interested in how to use the decoder, I left some code using the opus decoder commented out.
* To play the encoded data, I convert it back using the opus_demo executable and play the result
```
../00_preparation/opus/opus_demo  -d 48000 1 ../00_preparation/test_c1_48000_s16.opus ../00_preparation/test_c1_48000_s16_decoded.raw
play -t raw -e signed-integer -b 16 -c 1 -r 48000 ../00_preparation/test_c1_48000_s16_decoded.raw
```
The quality is much better. Not sure what goes wrong with the libopusenc example.

### Compile the opus based test program execute and play (ogg format)
The commands are summarized in 02_encode_on_host_opus_ogg/compile.sh

According to [RFC 7845 - Ogg Encapsulation for the Opus Audio Codec](https://datatracker.ietf.org/doc/html/rfc7845) we need
* A page with an Opus header. I set pre-skip, output gain and mapping family to 0. It is required to set the beginning of stream to true in the first page.
* A page with an Opus comment. I set vendor string to "ABC" and give no comments.
For the audio data packets, I create one page per encoder call.
After decoding finished, I write a footer where end of stream is true.

The take aways are
* Oggz has and interface that takes care of file handling when oggz_open() is used. Alternatively an Ogg stream can be created by oggz_new() and the output handling is done by a callback registered with oggz_io_set_write(). I use second to be able to stream the data later on Spresense.
* It is required to get the b_o_s, e_o_s and packetno correct.
* For granulepos, which is used to seek the audio, I set the number of frames.

## Encode a pre-recorded test file or life mic input on Spresense
### Compile the libraries for Spresense

Before compiling the libraries for Spresense, I would like to comment that the [arduino libopus library](https://github.com/pschatzmann/arduino-libopus) by Phil Schatzmann would work fine to integrate libopus into a Spresense sketch. The reason why we compile it ourselves is to enable optimizations. Depending on the configuration (e.g. complexity) we are crossing the boarder of real time. You can give it a try.

Set up the toolchain for Spresense according to https://developer.sony.com/develop/spresense/docs/sdk_set_up_en.html#_development_environment and activate it.

To cross compile, configure needs to know the target architecture. Also we need to give certain compiler and linker flags. I used the flags I found in the platform.txt of the Arduino configuration [spresense-arduino-compatible/platform.txt at master · sonydevworld/spresense-arduino-compatible · GitHub](https://github.com/sonydevworld/spresense-arduino-compatible/blob/master/Arduino15/packages/SPRESENSE/hardware/spresense/1.0.0/platform.txt)

The following changes were made
* add `-mfloat-abi=hard` (part of the Arduino compiler flags) to avoid liker error. soft float and hard float ABI may not be mixed.
* add `--host=arm-none-eabi` to tell configure to setup for compiling for Spresense
* add  `export LDFLAGS='--specs=nosys.specs'` to countermeasure 
```
exit.c:(.text.exit+0x2c): undefined reference to `_exit'
```
* add `--disable-rtcd` to countermeasure
```
celt/arm/armcpu.c:153:3: error: #error "Configured to use ARM asm but no CPU detection method available for " "your platform.  Reconfigure with --disable-rtcd (or send patches)."
```
* add `--enable-fixed-point --disable-float-api --enable-float-approx` to use the fixed point solution and no float calculations because this runs much faster on Spresense. (Spresense has a FPU, so why?) Using float API is with no configuration real-time. It is about 3 times slower. Not sure if the float approx is used or makes sense.
* add `--disable-doc --disable-extra-programs` to avoid compiling unecessary parts
* add `--prefix` to avoid installing to the host system.
* replace the `-ggdb -gdwarf-3` to `-s` to reduce size.
* removed the tools in liboggz/src/Makefile.in line 330 due to
```
httpdate.c:63:15: error: 'timezone' undeclared (first use in this function); did you mean '_timezone'?
   63 |   d.tm_sec -= timezone;
      |               ^~~~~~~~
      |               _timezone
```
* add `D_FORTIFY_SOURCE=0` to avoid a linker error in Arduino build for __memory_chk not found
* add `-mimplicit-it=thumb` to prevent "thumb conditional instruction should be in IT block" in Arduino build
* libopusenc is not used on Spresence. I tried it out and it seems to run fine. I just made the error and activated UsbMsc, which leads to no bytes written. I found this out later. I never checked the output. To make libopusenc compile in the Arduino build, I had to remove all asserts. Also it compiles only if the float API is enabled.
* after configure of opus, edit config.h and switch off OPUS_ARM_MAY_HAVE_NEON_INTR to prevent linker error in Arduino build.

```
source ~/spresenseenv/setup

export LDFLAGS='--specs=nosys.specs'

export CFLAGS=' -MMD -std=gnu11 -fno-builtin -mabi=aapcs -Wall -Os -fno-strict-aliasing -fno-strength-reduce -fomit-frame-pointer -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -pipe -s -gdwarf-3 -ffunction-sections -fdata-sections'

export CXXFLAGS=' -MMD -std=gnu++11 -c -fno-builtin -mabi=aapcs -Wall -Os -fno-strict-aliasing -fno-strength-reduce -fomit-frame-pointer -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -pipe -ffunction-sections -fno-exceptions -fno-rtti -s -ffunction-sections -fdata-sections -fpermissive'

export CPPFLAGS=' -D_FORTIFY_SOURCE=0 -Wa,-mimplicit-it=thumb'

cd opus
./configure --host=arm-none-eabi --prefix=$PWD/../outSpresense --disable-rtcd --enable-fixed-point --disable-float-api --enable-float-approx --disable-doc --disable-extra-programs
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
```

### Build the sample for Spresense
The Arduino project is in 03_encode_on_spresense_main.

You need to modify your Arduino SDK files to let the Arduino environment know about the opus, ogg and oggz libs and includes.
Add this to the boards.txt at the end of the line of `spresense.build.libs= ... "{build.libpath}/libxx.a"`. Replace <insert abolute path here> with the real path where you cloned this repository to.
```
"<insert abolute path here>/00_preparation/outSpresense/lib/libopus.a" "<insert abolute path here>/00_preparation/outSpresense/lib/libogg.a" "<insert abolute path here>/00_preparation/outSpresense/lib/liboggz.a"
```
Add this to the platform.txt at the end of the line of `compiler.cpreprocessor.flags= ...  -D__NuttX__`. 
```
"-I<insert abolute path here>/00_preparation/outSpresense/include" "-I<insert abolute path here>/00_preparation/outSpresense/include/opus" 
```
Adding the same parts for the suc core will be required for the sub core build in the next step.

If you have arduino-cli installed, use
```
arduino-cli compile --fqbn SPRESENSE:spresense:spresense --board-options "Core=Main,Memory=1152,Debug=Disabled,UploadSpeed=230400"
arduino-cli upload -p /dev/tty.SLAB_USBtoUART --fqbn SPRESENSE:spresense:spresense --board-options "Core=Main,Memory=1152,Debug=Disabled,UploadSpeed=230400" 
```
otherwise use the Arduino IDE (or Arduino for VS Code plugin) and
* set the Core to Main Core, Memory to 1152 and upload speed to 230400.

The sketch has the following defines
* USE_SD_TEST_INPUT selects if the audio data shall be read from SD card (modify testAudioFileNameIn and copy the data to the SD card) or read from the microphone (attach a microphone to the Spresense board)
* STREAM_AUDIO selects if the encoded output shall be written to a file (modify testAudioFileNameOgg and prepare the folder on SD card) or written to stdout (which is the Serial.print())

Further comments on the Sketch
* I set the analog mic gain to maximum, otherwise it is a very small volume
* I use a pthread to increase the stack size. opus encoder needs about 25KB of stack. Increasing the default stack size requires a modification in the Arduino SDK, so I use this workaround. 
* opus also needs about 30KB of heap. That will become important for the ASMP worker.
* If you forget the sleep in the main loop, the sketch will just freeze.

### Receive the stream and play it
To receive the stream via serial
* start ffmpeg in a console window and listen to UPD port 20003 with `ffplay -i udp://192.168.1.200:20003`
* start the python script 03_encode_on_spresense_main/scripts/serialProxy.py
	* It will open serial `/dev/tty.SLAB_USBtoUART` which will trigger a reset on Spresense
	* It will connect to UPD localhost port 20003
	* And then forward data from Serial to UDP

## Encode on Spresense using ASMP worker
I assume you already prepared the libraries and modifed the Arduino SDK as explained in the previous chapter.
In boards.txt add the part to the spresense.build.libs line which is used for the sub core.

We will use the AudioFFT sample as a base. The main core has the following modifications
* select 1 channel `const int mic_channel_num = 1;`
* set maximum mic gain `theAudio->setRecorderMode(AS_SETRECDR_STS_INPUTDEVICE_MIC, 210);`
* set the buffer_sample to 960 * mic_channel_num. Otherwise the encoder will crash `static const int32_t buffer_sample = 960 * mic_channel_num;`
* Remove all Serial.prints as we want to stream the encoded audio on Serial.

Compile with Memory set to 896KB to leave memory for the workers and audio buffers.

Comments on the sketch for the sub core
* Since opus encode tries to allocate around 30KB, use USER_HEAP_SIZE to increase the heap. The heap automatically increases up to a 128KB boundariy. This sketch will (unfotunately) use 386KB.
* Add a buffer for oggz. I used a vector that dyanmically grows when writing the ogg packages. The I take the data and write it to Serial. fprintf to stdout crashes for some reason on the sub core. So adding a Serial.begin is required.
* I do not use any buffer but directly feed the data to the opus encoder. Assumption is that it works in real time. I tried adapting the ring buffer, but it did not work as expected.

This sketch is more or less hardcoded for 1 channel 48KHz. It would be possible to augment the Capture struct with the required data and/or send an init stuct in advance.

I use 48KHz to avoid blocking a 128KB memory block for the downsampling DSP. Otherwise using 16KHz would propably make more sense. Opus downsamples internally.

To play the Serial output, see the previous chapter.

## Encode on Spresense using a DSP binary
Sketches are in 05_encode_on_spresense_dsp.

This is quite hard. It still does not work. 

The current (and maybe last?) issue is that it crashes when I create a binary that uses more than 256KB with ERROR: Stack pointer is not within the allocated stack. On the other side I could not reduce the size to under 256KB. I removed 
* the oggz 
* debug information from the elf file
* deactivated all prints 
* reduced all buffers I had to a minimum.

I already spent much time and will take a break. Maybe I will try again when I have a debugger ready to get more insight efficiently.
This discussion [Opus - SILK vs CELT - Nordic Q&A - Nordic DevZone - Nordic DevZone](https://devzone.nordicsemi.com/f/nordic-q-a/60654/opus---silk-vs-celt) suggest to reduce size by removing either SILK or CELT during compile time.

These are the in sights so far
* It is possible to build the DSP file with the Arduino IDE. Just do not take or upload the resulting spk file, but rename the resulting elf file to OPUSENC.
* Do not do a MP.begin() because this sends a bootup message with the value of 0. However the encoder checks for a bootup message with value DSP_OPUSENC_VERSION
which is defined in modules/audio/include/apus/dsp_audio_version.h as 0x010306
* Use a 2 as last parameter in mpmq_init as it is not ignored. Likely the reason the worker sample comments that it is ignored, is that it links a library that likely uses modules/asmp/worker/mpmq.c. However the one linked with the Arduino IDE seems to be modules/asmp/supervisor/mpmq.c. I am not exactly sure why it works with a 2. I assume 2 is mapped to the main CPU, so it sends messages there. Trying to use the same lib as in the worker example give a lot of linker errors.
* The way of communication is similar to the user defined preprocessor dsp (see spresense/examples/audio_player_objif/worker). There are the commands
	* Init with the initialization parameters of the encoder. I observed that command can arrive twice.
	* Exec with an input and output buffer for the encoder
	* Flush with an output buffer for the encoder to finalize final data or write a footer.
* The data is defined in the namespace Wien2::Apu by commands ApuInitEncCmd, ApuExecEncCmd and ApuFlushEncCmd inside the file modules/audio/include/apus/apu_cmd.h. Commands are send via an address that you need to cast to the correct struct.
* As a response ApuExecOK is expected in result.exec_result
* The same data structure (pointer) is send back. As the origin is a physical addres, no conversion is needed.

Building within Spresense SDK based on a modified worker within the audio_player_objif example. 
It was really hard to get it linked. 
	* Finally I gave up using ld to link the final binary, but switched to g++ which is what also Arduino does. g++ automatically links required parts of the standard library required for e.g. new or malloc and passes certain options for linking. 
	* I introduced the compile flags I used in the Arduino environment.
	* It seems to be important to give the libraries in right order to the linker. This can be mitigated by using -Wl,--start-group and -Wl,--end-group.
	* STACK_SIZE needs to be passed to the linker as symbol. The size of 0x8000 makes me assume that this is heap and stack. When using g++ it can be defined by using -Wl like -Wl,--defsym,STACK_SIZE=0x8000
	* There is a definition of max-page-size=256. Maybe that gives a hint that the a DSP really may not exceed 256KB.
	* The linker complained that the symbol "end" required by sbrk is not defined. According to [undefined reference to `end' in sbrk.c in library libnosys.a !! - Code Composer Studio  forum - Code Composer Studio™︎ - TI E2E support forums](https://e2e.ti.com/support/tools/code-composer-studio-group/ccs/f/code-composer-studio-forum/382293/undefined-reference-to-end-in-sbrk-c-in-library-libnosys-a) it needs to point to the end of the heap. So I modified sdk/tools/asmp-elf.ld by adding end = __stack; at the end of .bss. Not sure if that is really correct. Spresense only defines a heapstack. End of heap is likely __stack minus stack size.
	* To include the DSP binary in the build. EXAMPLES_AUDIO_RECORDER_OBJIF_USEPREPROC needs to be conigured by SDK config.

I put the worker folder into the repository under 06_DSP_built_with_SDK, however it does not work and I proceeded with the Arduino build. It is there just for reference, in case you want to try finishing this task.


Thanks to Phil Schatzmann for making the arduino audio tools and the [arduino libopus library](https://github.com/pschatzmann/arduino-libopus) available.
