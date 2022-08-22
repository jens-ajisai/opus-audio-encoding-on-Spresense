#include <MediaRecorder.h>
#include <MemoryUtil.h>
#include <SDHCI.h>

#include <vector>

#include "utils.h"
#include "config.h"

extern SDClass theSD;
MediaRecorder *theRecorder;

static bool mediarecorder_done_callback(AsRecorderEvent event, uint32_t result,
                                        uint32_t sub_result) {
  if (DEBUG_LOG) printf("mp cb %x %lx %lx\n", event, result, sub_result);
  return true;
}

static void mediarecorder_attention_cb(const ErrorAttentionParam *atprm) {
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING) {
    printTimestamp(&Serial);
    printf("mediarecorder_attention_cb: error %d\n", atprm->error_code);
    if (atprm->error_att_sub_code ==
        AS_ATTENTION_SUB_CODE_SIMPLE_FIFO_OVERFLOW) {
      printf("AS_ATTENTION_SUB_CODE_SIMPLE_FIFO_OVERFLOW\n");
    }
  }
}

void beginRecorder(int recoding_cannel_number, int recoding_sampling_rate,
                   int recoding_bit_length, int analog_mic_gain) {
  if (initMemoryPools()) {
    printf("Error initializing memory pools\n");
  }
  err_t err = createStaticPools(MEM_LAYOUT_RECORDER);
  if (err) {
    printf("Error createStaticPools: %d\n", err);
  }

  theRecorder = MediaRecorder::getInstance();

  err = theRecorder->begin(mediarecorder_attention_cb);
  if (err) {
    printf("Error theRecorder->begin: %d\n", err);
  }

  bool success = theRecorder->setCapturingClkMode(MEDIARECORDER_CAPCLK_NORMAL);
  if (!success) {
    printf("Error theRecorder->setCapturingClkMode: %d\n", err);
  }

  err = theRecorder->activate(AS_SETRECDR_STS_INPUTDEVICE_MIC,
                              mediarecorder_done_callback);
  if (err) {
    printf("theAudio->activate: %d\n", err);
  }

  usleep(10 * 1000);  // waiting for Mic startup

  err = theRecorder->init(AS_CODECTYPE_WAV, recoding_cannel_number,
                          recoding_sampling_rate, recoding_bit_length, AS_BITRATE_96000,
                          "/mnt/spif/BIN");
  if (err) {
    printf("theAudio->initRecorder: %d\n", err);
  }

  theRecorder->setMicGain(analog_mic_gain);

  theRecorder->start();
  printf("theRecorder->start successful");
}

void endRecorder() {
  printf("theRecorder->stop!!!!");
  theRecorder->stop();
  theRecorder->deactivate();
  theRecorder->end();
}

size_t getAudio(uint8_t *buffer, size_t length) {
  uint32_t read_size = 0;
  err_t err = theRecorder->readFrames(buffer, length, &read_size);
  if (err != MEDIARECORDER_ECODE_OK &&
      err != MEDIARECORDER_ECODE_INSUFFICIENT_BUFFER_AREA) {
    printf("Error while reading frames. err %d\n", err);
  }
  return read_size;
}
