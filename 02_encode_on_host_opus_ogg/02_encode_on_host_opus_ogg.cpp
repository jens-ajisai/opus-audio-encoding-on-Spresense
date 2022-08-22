#include <stdint.h>
#include <vector>

#include "encoderPoc_ogg.h"
#include "encoderPoc_opus.h"

#define DEBUG_LOG 1

#define READ_SIZE_FROM_FILE 480

// OPUS encoder

#define AS_SAMPLINGRATE_48000 48000
#define CHANNEL_1CH 1
#define AS_BITLENGTH_16 16

int granulepos = 0;

// files
FILE *testAudioIn;
FILE *testAudioOutOgg;
const char *testAudioFileNameIn = "../00_preparation/test_c1_48000_s16.raw";
const char *testAudioFileNameOgg = "../00_preparation/test_c1_48000_s16.ogg";

uint32_t recoding_sampling_rate = AS_SAMPLINGRATE_48000;
uint8_t recording_cannel_number = CHANNEL_1CH;
uint8_t recoding_bit_length = AS_BITLENGTH_16;

static std::vector<uint8_t> audioBuffer;

void writeOggFormat(std::vector<uint8_t> &encodedBytes, size_t encodedBytesLen,
                    int pos) {
  ogg_writeAudioDataPage(encodedBytes.data(), encodedBytesLen, pos);
}

void handleAudio(OpusAudioEncoder &opusEncoder) {
  if (DEBUG_LOG) printf("enter handleAudio\n");

  testAudioIn = fopen(testAudioFileNameIn, "rb");

  audioBuffer.resize(opusEncoder.getExpectedPackageSize());

  while (true) {
    size_t readBytes =
        fread(audioBuffer.data(), 1, audioBuffer.size(), testAudioIn);
    if (readBytes == audioBuffer.size()) {
      size_t encodedBytesLen;
      std::vector<uint8_t> encodedBytes = opusEncoder.encodeFrame(
          audioBuffer.data(), readBytes, &encodedBytesLen);

      granulepos +=
          (audioBuffer.size() / sizeof(int16_t) / recording_cannel_number);
      writeOggFormat(encodedBytes, encodedBytesLen, granulepos);
    } else {
      break;
    }
  }

  fclose(testAudioIn);
}

int main() {
  ogg_begin(testAudioFileNameOgg);
  ogg_writeHeaderPage(recording_cannel_number, recoding_sampling_rate);

  OpusEncoderSettings cfg(recording_cannel_number, recoding_sampling_rate,
                          recoding_bit_length);
  OpusAudioEncoder opusEncoder(cfg);

  handleAudio(opusEncoder);

  ogg_writeFooterPage();
  ogg_end();
}
