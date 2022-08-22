#include <SDHCI.h>

#include <vector>

#include "encoderPoc_mic.h"
SDClass theSD;

#define AS_SAMPLINGRATE_48000 48000
#define CHANNEL_1CH 1
#define AS_BITLENGTH_16 16

uint32_t recording_sampling_rate = AS_SAMPLINGRATE_48000;
uint8_t recording_cannel_number = CHANNEL_1CH;
uint8_t recording_bit_length = AS_BITLENGTH_16;
uint8_t analog_mic_gain = 210;

static std::vector<uint8_t> audioBuffer;

void handleAudio() {
  size_t readBytes = getAudio(audioBuffer.data(), audioBuffer.size());
}

int _main() { return 0; }

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  while (!theSD.begin()) {
  }

  audioBuffer.resize(480 * 8);

  beginRecorder(recording_cannel_number, recording_sampling_rate, recording_bit_length,
                analog_mic_gain);
}

#define RECORDING_TIME (10 * 1000)

void loop() {
  handleAudio();

  if (millis() > RECORDING_TIME) {
    endRecorder();
    exit(0);
  }
}
