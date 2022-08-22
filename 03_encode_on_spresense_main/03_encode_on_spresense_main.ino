#include <stdint.h>

#include <string>
#include <vector>

#include "config.h"
#include "encoderPoc_mic.h"
#include "encoderPoc_ogg.h"
#include "encoderPoc_opus.h"

#define USE_SD_TEST_INPUT 0
#define STREAM_AUDIO 1

#include <SDHCI.h>
SDClass theSD;

#define READ_SIZE_FROM_FILE 480

#define AS_SAMPLINGRATE_48000 48000
#define CHANNEL_1CH 1
#define AS_BITLENGTH_16 16

int posInSamples = 0;

// files
FILE *testAudioIn;
FILE *testAudioOutOgg;
FILE *testAudioDecoded;
const char *testAudioFileNameIn = "/mnt/sd0/audio/test_c1_48000_s16.raw";
const char *testAudioFileNameOgg = "/mnt/sd0/audio/test_c1_48000_s16.ogg";

uint32_t recording_sampling_rate = AS_SAMPLINGRATE_48000;
uint8_t recording_cannel_number = CHANNEL_1CH;
uint8_t recording_bit_length = AS_BITLENGTH_16;
uint8_t analog_mic_gain = 210;

static std::vector<uint8_t> audioBuffer;

void writeOggFormat(std::vector<uint8_t> &encodedBytes, size_t encodedBytesLen, int pos) {
  ogg_writeAudioDataPage(encodedBytes.data(), encodedBytesLen, pos);
}

void handleAudio(OpusAudioEncoder &opusEncoder) {
  if (DEBUG_LOG) printf("enter handleAudio\n");

  audioBuffer.resize(opusEncoder.getExpectedPackageSize());

  while (true) {
#if USE_SD_TEST_INPUT
    size_t readBytes = fread(audioBuffer.data(), 1, audioBuffer.size(), testAudioIn);
    if (DEBUG_LOG) printf("read %d bytes of %d from file.\n", readBytes, audioBuffer.size());
#else
    size_t readBytes = getAudio(audioBuffer.data(), audioBuffer.size());
    if (DEBUG_LOG) printf("read %d bytes of %d from recorder.\n", readBytes, audioBuffer.size());
#endif

    if (readBytes == audioBuffer.size()) {
      size_t encodedBytesLen;

      std::vector<uint8_t> encodedBytes =
          opusEncoder.encodeFrame(audioBuffer.data(), readBytes, &encodedBytesLen);

      posInSamples += (audioBuffer.size() / sizeof(int16_t) / recording_cannel_number);
      writeOggFormat(encodedBytes, encodedBytesLen, posInSamples);
    } else {
#if USE_SD_TEST_INPUT
      printf("End of SD input.\n");
      break;
#else
      if (DEBUG_LOG) printf("Not yet ready.\n");
#endif
    }
  }
}

int _main() {
  if (DEBUG_LOG)
    printf("open files %s %s\n", testAudioFileNameIn, testAudioFileNameOgg);

#if USE_SD_TEST_INPUT
  testAudioIn = fopen(testAudioFileNameIn, "rb");
#endif
#if STREAM_AUDIO
  testAudioOutOgg = stdout;
#else
  testAudioOutOgg = fopen(testAudioFileNameOgg, "wb");
#endif

  if ((USE_SD_TEST_INPUT && testAudioIn == NULL) || testAudioOutOgg == NULL) {
    printf("Failed to open one of the files!");
    return;
  }

  ogg_begin(testAudioOutOgg);
  ogg_writeHeaderPage(recording_cannel_number, recording_sampling_rate);

  OpusEncoderSettings cfg(recording_cannel_number, recording_sampling_rate);
  cfg.complexity = 0;
  OpusAudioEncoder opusEncoder(cfg);

#if USE_SD_TEST_INPUT == 0
  beginRecorder(recording_cannel_number, recording_sampling_rate, recording_bit_length,
                analog_mic_gain);
#endif

  handleAudio(opusEncoder);

#if USE_SD_TEST_INPUT == 0
  endRecorder();
#endif

  ogg_writeFooterPage();
  ogg_end();

#if USE_SD_TEST_INPUT
  fclose(testAudioIn);
#endif
#if STREAM_AUDIO == 0
  fflush(testAudioOutOgg);
  fclose(testAudioOutOgg);
#endif

  return 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  while (!theSD.begin()) {
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  attr.stacksize = 28000;  // 24xxx required
  pthread_t handleAudio_id;
  pthread_create(&handleAudio_id, &attr, (pthread_startroutine_t)_main, NULL);

  pthread_setname_np(handleAudio_id, "handleAudio");
}

void loop() {
  // very important to sleep!!!
  sleep(1);
}
