#pragma once

#include <vector>

#include "opus/opus.h"

#include "worker_config.h"

class OpusAudioEncoder {
 public:
  OpusAudioEncoder(int recording_cannel_number, int recording_sampling_rate) {
    int err;

    enc = opus_encoder_create(recording_sampling_rate, recording_cannel_number, OPUS_APPLICATION_VOIP,
                              &err);
    if (err != OPUS_OK) {
      printf("failed creating encoder with error %s for parameter(%d %d %d)\n",
             opus_strerror(err), recording_sampling_rate,recording_cannel_number ,OPUS_APPLICATION_VOIP);
    } 
  }

  ~OpusAudioEncoder() { opus_encoder_destroy(enc); }

  int getFinalRange() {
    opus_uint32 enc_final_range;
    opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range));
    return enc_final_range;
  }

  int encodeFrame(uint8_t *dataIn, size_t dataInLen, uint8_t *dataOut, size_t dataOutLenMax) {
    int frames = dataInLen /  1 / sizeof(int16_t);

    int len = opus_encode(enc, (opus_int16 *)dataIn, frames,
                          dataOut, dataOutLenMax);
    if (len < 0) {
      printf("opus_encode error: %s\n", opus_strerror(len));
      printf("opus_encode(..., dataLen: %u, frames: %d, max_data_bytes: %d)\n",
             dataInLen, frames, dataOutLenMax);
    } else if (len > 0 && len <= dataOutLenMax) {
      if (DEBUG_LOG) printf("opus_encode: %d bytes\n", len);
    }
    return len;
  }

 private:
  OpusEncoder *enc;

  int frame_pos = 0;

};