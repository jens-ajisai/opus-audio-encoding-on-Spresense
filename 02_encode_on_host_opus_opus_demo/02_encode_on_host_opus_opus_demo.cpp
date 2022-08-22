#include <stdint.h>
#include <vector>

#include "encoderPoc_opus.h"

#define DEBUG_LOG 1

#define READ_SIZE_FROM_FILE 480

// OPUS encoder

#define AS_SAMPLINGRATE_48000 48000
#define CHANNEL_1CH 1
#define AS_BITLENGTH_16 16

// files
FILE *testAudioIn;
FILE *testAudioOutOpus;
const char *testAudioFileNameIn = "../00_preparation/test_c1_48000_s16.raw";
const char *testAudioFileNameOut = "../00_preparation/test_c1_48000_s16.opus";

uint32_t recording_sampling_rate = AS_SAMPLINGRATE_48000;
uint8_t recording_cannel_number = CHANNEL_1CH;
uint8_t recording_bit_length = AS_BITLENGTH_16;

static void int_to_char(opus_uint32 i, unsigned char ch[4]) {
  ch[0] = i >> 24;
  ch[1] = (i >> 16) & 0xFF;
  ch[2] = (i >> 8) & 0xFF;
  ch[3] = i & 0xFF;
}

static std::vector<uint8_t> audioBuffer;

void writeOpusDemoFormat(FILE *opusOut, std::vector<uint8_t> &encodedBytes,
                         size_t encodedBytesLen, uint32_t encFinalRange) {
  unsigned char int_field[4];

  int_to_char(encodedBytesLen, int_field);
  fwrite(int_field, 1, 4, opusOut);

  int_to_char(encFinalRange, int_field);
  fwrite(int_field, 1, 4, opusOut);

  fwrite(encodedBytes.data(), 1, encodedBytesLen, opusOut);
}

void handleAudio(OpusAudioEncoder &opusEncoder) {
  if (DEBUG_LOG) printf("enter handleAudio\n");

  testAudioIn = fopen(testAudioFileNameIn, "rb");
  testAudioOutOpus = fopen(testAudioFileNameOut, "wb");

  audioBuffer.resize(opusEncoder.getExpectedPackageSize());

  while (true) {
    size_t readBytes =
        fread(audioBuffer.data(), 1, audioBuffer.size(), testAudioIn);
    if (readBytes == audioBuffer.size()) {
      size_t encodedBytesLen;
      std::vector<uint8_t> encodedBytes = opusEncoder.encodeFrame(
          audioBuffer.data(), readBytes, &encodedBytesLen);
      writeOpusDemoFormat(testAudioOutOpus, encodedBytes, encodedBytesLen,
                          opusEncoder.getFinalRange());
    } else {
      break;
    }
  }

  fclose(testAudioIn);
  fflush(testAudioOutOpus);
  fclose(testAudioOutOpus);
}

int main() {
  OpusEncoderSettings cfg(recording_cannel_number, recording_sampling_rate,
                          recording_bit_length);
  OpusAudioEncoder opusEncoder(cfg);

  handleAudio(opusEncoder);
}
