#pragma once

#include <vector>

#include "opus/opus.h"

#define DEBUG_LOG 1

#define OPUS_MAX_BUFFER_SIZE (5760)

class OpusEncoderSettings {
 public:
  OpusEncoderSettings(int recording_cannel_number, int recording_sampling_rate,
                      int recording_bit_length) {
    channels = recording_cannel_number;
    sample_rate = recording_sampling_rate;
    bits_per_sample = recording_bit_length;
  }

  OpusEncoderSettings() { OpusEncoderSettings(1, 48000, 16); }

  int channels = 2;
  int application = OPUS_APPLICATION_VOIP;
  int sample_rate = 48000;
  int bits_per_sample = 16;
  int max_buffer_size = OPUS_MAX_BUFFER_SIZE;
  int complexity = 10;
  int frame_sizes_ms = OPUS_FRAMESIZE_10_MS;

  // not used
  int bitrate = -1;
  int force_channel = -1;
  int use_vbr = -1;
  int vbr_constraint = -1;
  int max_bandwidth = -1;
  int singal = -1;
  int inband_fec = -1;
  int packet_loss_perc = -1;
  int lsb_depth = -1;
  int prediction_disabled = -1;
  int use_dtx = -1;
};

class OpusAudioEncoder {
 public:
  OpusAudioEncoder(OpusEncoderSettings _cfg) {
    cfg = _cfg;

    int err;
    encodedDataPacket.resize(cfg.max_buffer_size);
    //    decoded.resize(getFrameSizeSamples(cfg.sample_rate) *
    //    sizeof(int16_t));
    assert(encodedDataPacket.data() != nullptr);
    //    assert(decoded.data() != nullptr);

    if (DEBUG_LOG)
      printf("create encoder with %d %d %d\n", cfg.sample_rate, cfg.channels,
             cfg.application);

    enc = opus_encoder_create(cfg.sample_rate, cfg.channels, cfg.application,
                              &err);
    if (err != OPUS_OK) {
      printf("failed creating encoder with error %s for parameter(%d %d %d)\n",
             opus_strerror(err), cfg.sample_rate, cfg.channels,
             cfg.application);
      return;
    }

    //    dec = opus_decoder_create(cfg.sample_rate, cfg.channels, &err);

    applyConfiguration();
  }

  ~OpusAudioEncoder() { opus_encoder_destroy(enc); }

  OpusEncoderSettings &config() { return cfg; }

  size_t getExpectedPackageSize() {
    return getFrameSizeSamples(cfg.sample_rate) * sizeof(int16_t);
  }

  int getFinalRange() {
    opus_uint32 enc_final_range;
    opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range));
    return enc_final_range;
  }

  std::vector<uint8_t> encodeFrame(uint8_t *data, size_t dataLen, size_t *dataLenOut) {
    int frames = dataLen / cfg.channels / sizeof(int16_t);
    if (DEBUG_LOG)
      printf("opus_encode(..., dataLen: %lu, frames: %d, max_data_bytes: %d)\n",
             dataLen, frames, cfg.max_buffer_size);
    int len = opus_encode(enc, (opus_int16 *)data, frames,
                          encodedDataPacket.data(), cfg.max_buffer_size);
    if (len < 0) {
      printf("opus_encode error: %s\n", opus_strerror(len));
      printf("opus_encode(..., dataLen: %lu, frames: %d, max_data_bytes: %d)\n",
             dataLen, frames, cfg.max_buffer_size);
    } else if (len > 0 && len <= cfg.max_buffer_size) {
      if (DEBUG_LOG) printf("opus_encode: %d bytes\n", len);
    }

    *dataLenOut = len;
    return encodedDataPacket;

    //    int decodedSize = opus_decode(dec, encodedDataPacket.data(), len,
    //                                  (opus_int16 *)decoded.data(), lenBytes,
    //                                  0);
    //    if (decodedSize < 0) {
    //      printf("opus_decode error: %s\n", opus_strerror(len));
    //    } else if (len > 0 && len <= cfg.max_buffer_size) {
    //      fwrite(decoded.data(), 1, decodedSize * 2, testAudioDecoded);
    //    }
  }

 private:
  OpusEncoder *enc;
  //  OpusDecoder *dec;
  OpusEncoderSettings cfg;
  std::vector<uint8_t> encodedDataPacket;
  //  std::vector<uint8_t> decoded;
  int frame_pos = 0;

  // Returns the frame size in samples
  int getFrameSizeSamples(int sampling_rate) {
    switch (cfg.frame_sizes_ms) {
      case OPUS_FRAMESIZE_2_5_MS:
        return sampling_rate / 400;
      case OPUS_FRAMESIZE_5_MS:
        return sampling_rate / 200;
      case OPUS_FRAMESIZE_10_MS:
        return sampling_rate / 100;
      case OPUS_FRAMESIZE_20_MS:
        return sampling_rate / 50;
      case OPUS_FRAMESIZE_40_MS:
        return sampling_rate / 25;
      case OPUS_FRAMESIZE_60_MS:
        return 3 * sampling_rate / 50;
      case OPUS_FRAMESIZE_80_MS:
        return 4 * sampling_rate / 50;
      case OPUS_FRAMESIZE_100_MS:
        return 5 * sampling_rate / 50;
      case OPUS_FRAMESIZE_120_MS:
        return 6 * sampling_rate / 50;
    }
    return sampling_rate / 100;
  }

  bool applyConfiguration() {
    bool ok = true;
    if (cfg.bitrate >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_BITRATE(cfg.bitrate)) != OPUS_OK) {
      printf("invalid bitrate: %d\n", cfg.bitrate);
      ok = false;
    }
    if (cfg.force_channel >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(cfg.force_channel)) !=
            OPUS_OK) {
      printf("invalid force_channel: %d\n", cfg.force_channel);
      ok = false;
    };
    if (cfg.use_vbr >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_VBR(cfg.use_vbr)) != OPUS_OK) {
      printf("invalid vbr: %d\n", cfg.use_vbr);
      ok = false;
    }
    if (cfg.vbr_constraint >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(cfg.vbr_constraint)) !=
            OPUS_OK) {
      printf("invalid vbr_constraint: %d\n", cfg.vbr_constraint);
      ok = false;
    }
    if (cfg.complexity >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(cfg.complexity)) != OPUS_OK) {
      printf("invalid complexity: %d\n", cfg.complexity);
      ok = false;
    }
    if (cfg.max_bandwidth >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_MAX_BANDWIDTH(cfg.max_bandwidth)) !=
            OPUS_OK) {
      printf("invalid max_bandwidth: %d\n", cfg.max_bandwidth);
      ok = false;
    }
    if (cfg.singal >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_SIGNAL(cfg.singal)) != OPUS_OK) {
      printf("invalid singal: %d\n", cfg.singal);
      ok = false;
    }
    if (cfg.inband_fec >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(cfg.inband_fec)) != OPUS_OK) {
      printf("invalid inband_fec: %d\n", cfg.inband_fec);
      ok = false;
    }
    if (cfg.packet_loss_perc >= 0 &&
        opus_encoder_ctl(
            enc, OPUS_SET_PACKET_LOSS_PERC(cfg.packet_loss_perc)) != OPUS_OK) {
      printf("invalid pkt_loss: %d\n", cfg.packet_loss_perc);
      ok = false;
    }
    if (cfg.lsb_depth >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(cfg.lsb_depth)) != OPUS_OK) {
      printf("invalid lsb_depth: %d\n", cfg.lsb_depth);
      ok = false;
    }
    if (cfg.prediction_disabled >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_PREDICTION_DISABLED(
                                  cfg.prediction_disabled)) != OPUS_OK) {
      printf("invalid pred_disabled: %d\n", cfg.prediction_disabled);
      ok = false;
    }
    if (cfg.use_dtx >= 0 &&
        opus_encoder_ctl(enc, OPUS_SET_DTX(cfg.use_dtx)) != OPUS_OK) {
      printf("invalid use_dtx: %d\n", cfg.use_dtx);
      ok = false;
    }

    // Do not set OPUS_SET_EXPERT_FRAME_DURATION.
    // Frame size will be variable then.

    if (DEBUG_LOG)
      printf(
          "bitrate:%d, force_channel:%d, use_vbr:%d, vbr_constraint:%d, "
          "complexity:%d, max_bandwidth:%d, singal:%d, inband_fec:%d, "
          "packet_loss_perc:%d "
          "lsb_depth:%d, prediction_disabled:%d, use_dtx:%d\n",
          cfg.bitrate, cfg.force_channel, cfg.use_vbr, cfg.vbr_constraint,
          cfg.complexity, cfg.max_bandwidth, cfg.singal, cfg.inband_fec,
          cfg.packet_loss_perc, cfg.lsb_depth, cfg.prediction_disabled,
          cfg.use_dtx);
    return ok;
  }
};