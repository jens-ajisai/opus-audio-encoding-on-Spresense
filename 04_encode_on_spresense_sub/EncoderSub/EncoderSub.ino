/*
 *  SubFFT.ino - MP Example for Audio FFT
 *  Copyright 2019 Sony Semiconductor Solutions Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SUBCORE
#error "Core selection is wrong!!"
#endif

#include <MP.h>
#include <stdint.h>

#include <vector>

#include "encoderPoc_ogg.h"
#include "encoderPoc_opus.h"
#include "worker_config.h"

#define INCREASED_STACK_SIZE_IN_SDK 1

// Encoder instance used across init, exec and flush
OpusAudioEncoder *opusEncoder;

// Track the position for the ogg header
uint32_t posInSamples;

// need 30KB!! + maybe much more
USER_HEAP_SIZE(64 * 1024);


#define OPUS_ENC_FRAMESIZE_SAMPLES 960
// could be smaller
#define OPUS_OUT_MAX_SIZE 256

uint8_t pDst[OPUS_OUT_MAX_SIZE];
std::vector<uint8_t> oggzBuffer;

#define AS_SAMPLINGRATE_48000 48000
#define CHANNEL_1CH 1
#define AS_BITLENGTH_16 16

uint32_t recoding_sampling_rate = AS_SAMPLINGRATE_48000;
uint8_t recoding_cannel_number = CHANNEL_1CH;
uint8_t recoding_bit_length = AS_BITLENGTH_16;

/* MultiCore definitions */

struct Capture {
  void *buff;
  int sample;
  int chnum;
};


void begin() {
  ogg_begin(&oggzBuffer);
  ogg_writeHeaderPage(recoding_cannel_number, recoding_sampling_rate);
  opusEncoder = new OpusAudioEncoder(recoding_cannel_number, recoding_sampling_rate);
}

void exec(uint8_t * buf, int size) {
  if (DEBUG_LOG) printf("exec\n");
  if (size != OPUS_ENC_FRAMESIZE_SAMPLES * 2) {
    if (ERROR_LOG) printf("size does not match %d\n", OPUS_ENC_FRAMESIZE_SAMPLES);
  }
  int encodedSize = opusEncoder->encodeFrame(buf, size, pDst, OPUS_OUT_MAX_SIZE);
  posInSamples += (OPUS_ENC_FRAMESIZE_SAMPLES / sizeof(int16_t) / recoding_cannel_number);

  ogg_writeAudioDataPage(pDst, encodedSize, posInSamples);

  Serial.write(oggzBuffer.data(), oggzBuffer.size());
  oggzBuffer.clear();
}

void __main() {
  int ret = 0;

  Serial.begin(115200);
  while (!Serial);

  begin();
  /* Initialize MP library */
  ret = MP.begin();

  /* receive with non-blocking */
  MP.RecvTimeout(MP_RECV_POLLING);

  while (true) {
    int8_t msgid;
    Capture *capture;

    /* Receive PCM captured buffer from MainCore */
    ret = MP.Recv(&msgid, &capture);
    if (ret >= 0) {
      exec((uint8_t *)capture->buff, capture->sample * sizeof(uint16_t));
    }
  }
}

void setup() {
#if INCREASED_STACK_SIZE_IN_SDK  
    __main();
#else
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  attr.stacksize = 25000;
  pthread_t handleAudio_id;
  pthread_create(&handleAudio_id, &attr, (pthread_startroutine_t)__main, NULL);
#endif
}

void loop() { sleep(1); }
