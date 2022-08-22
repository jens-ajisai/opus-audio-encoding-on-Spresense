#include "worker_config.h"

#include <errno.h>
#include <memutils/common_utils/common_assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

extern "C" {
#include <asmp/mpmq.h>
#include <asmp/mpmutex.h>
#include <asmp/mpshm.h>
#include <asmp/types.h>
}

#include <stdint.h>

#include <vector>

#include "apus/apu_cmd.h"
#include "apus/apu_cmd_defs.h"
#include "apus/dsp_audio_version.h"
#include "dsp_driver/include/dsp_drv.h"
#if USE_OGG
#include "encoderPoc_ogg.h"
#endif
#include "encoderPoc_opus.h"

#define KEY_MQ 2
#define COMMAND_DATATYPE_ADDRESS 0
#define COMMAND_DATATYPE_VALUE 1

#define MSGID_PROCMODE_SHIFT 4
#define MSGID_DATATYPE_MASK 0x01

#define CRE_MSGID(mode, type) ((mode << MSGID_PROCMODE_SHIFT) | (type & MSGID_DATATYPE_MASK))

static mpmq_t s_mq;
static size_t bytesWritten;
static bool initialized = false;
extern "C" {

// copy from components/encoder/encoder_component.h because including headers is annoying for now
// due to further include dependencies which I do not need.

__WIEN2_BEGIN_NAMESPACE

typedef bool (*EncCompCallback)(void *);

struct InitEncParam {
  AudioCodec codec_type;
  uint32_t input_sampling_rate;
  uint32_t output_sampling_rate;
  AudioPcmFormat bit_width;
  uint8_t channel_num;
  uint8_t complexity;
  uint32_t bit_rate;
  EncCompCallback callback;
};

struct ExecEncParam {
  BufferHeader input_buffer;
  BufferHeader output_buffer;
};

struct StopEncParam {
  BufferHeader output_buffer;
};

struct EncCmpltParam {
  Wien2::Apu::ApuEventType event_type;
  bool result;

  union {
    InitEncParam init_enc_cmplt;
    ExecEncParam exec_enc_cmplt;
    StopEncParam stop_enc_cmplt;
  };
};

__WIEN2_END_NAMESPACE

// Start of my part

/*--------------------------------------------------------------------*/
void reply_to_spu(void *addr) {
  uint8_t msg_id = 0;
  uint32_t msg_data = 0;
  // Create message ID and send
  msg_id = CRE_MSGID(Wien2::Apu::EncMode, COMMAND_DATATYPE_ADDRESS);
  if (DEBUG_LOG) printf("DSP send msg_id %d addr %p.\n", msg_id, addr);
  //  int ret = MP.Send(msg_id, addr, 0);

  msg_data = reinterpret_cast<uint32_t>(addr);

  int ret = mpmq_send(&s_mq, msg_id, msg_data);
  if (ret != 0) {
    if (ERROR_LOG)
      printf("Error sending the reply message to the encoder component on main core.\n");
  }
}
};

#define INCREASED_STACK_SIZE_IN_SDK 1

// Encoder instance used across init, exec and flush
OpusAudioEncoder *opusEncoder;

#if USE_OGG
// Track the position for the ogg header
uint32_t posInSamples;
std::vector<uint8_t> oggzBuffer;
#endif

/*--------------------------------------------------------------------*/
int __main() {
  int ret = 0;

  // It is recommended to switch off loggings because they are printed without synchronisation to
  // the Serial prints of the main core
  if (DEBUG_LOG) printf("Hello from Opus Encoder DSP\n");

  // initialize the message queue
  //  IMPORTANT: Comment out the boot complete message in MP.cpp because the encoder_component
  //  checks for the value DSP_OPUSENC_VERSION being sent with the boot complete message!

  ret = mpmq_init(&s_mq, KEY_MQ, 2);
  if (ret != 0) {
    if (ERROR_LOG) printf("Error mpmq_init. %d\n", ret);
  }

  // Reply "boot complete" with the value DSP_OPUSENC_VERSION
  uint8_t msg_id = CRE_MSGID(Wien2::Apu::CommonMode, COMMAND_DATATYPE_VALUE);
  ret = mpmq_send(&s_mq, msg_id, DSP_OPUSENC_VERSION);

  if (ret != 0) {
    if (ERROR_LOG) printf("Error sending boot complete. %d\n", ret);
  }

  // Execution loop
  if (DEBUG_LOG) printf("Enter execution loop.\n");
  while (true) {
    int command = 0;
    uint32_t msgdata = 0;

    // Infinitely wait for messages from encoder component.
    // Messages could be
    //   * Init
    //   * Exec
    //   * Flush
    command = mpmq_receive(&s_mq, &msgdata);
    uint8_t type = command & MSGID_DATATYPE_MASK;

    if (DEBUG_LOG)
      printf("Received Message: command=%d, type=%d, msgdata=%p\n", command, type, (void *)msgdata);

    // Parse and execute message. In any case the message data must be an address.
    if (type == COMMAND_DATATYPE_ADDRESS) {
      Wien2::Apu::Wien2ApuCmd *com_param = reinterpret_cast<Wien2::Apu::Wien2ApuCmd *>(msgdata);

      if (DEBUG_LOG)
        printf("\tprocess_mode=%d, event_type=%d\n", com_param->header.process_mode,
               com_param->header.event_type);

      if (com_param->header.process_mode == Wien2::Apu::EncMode &&
          com_param->header.event_type == Wien2::Apu::InitEvent) {
        if (!initialized) {
          // Unpack the init command
          Wien2::Apu::ApuInitEncCmd initCmd = com_param->init_enc_cmd;

          if (DEBUG_LOG) printf("Process ApuInitEncCmd\n");
          /*
            ignore for now
              initCmd.output_sampling_rate
              initCmd.bit_rate
              initCmd.init_opus_enc_param.use_original_format
              initCmd.debug_dump_info
              initCmd.size
          */

          if (initCmd.codec_type != Wien2::AudCodecOPUS) {
            com_param->result.exec_result = Wien2::Apu::ApuExecError;
          }

          // The frame size is fixed to the recommended 20ms.
          OpusEncoderSettings cfg(initCmd.channel_num, initCmd.input_sampling_rate);
          cfg.complexity = initCmd.init_opus_enc_param.complexity;
          cfg.frame_sizes_ms = OPUS_FRAMESIZE_20_MS;
          opusEncoder = new OpusAudioEncoder(cfg);

#if USE_OGG
          // Give the ogg component a buffer to write the packages
          // Expect that packages are flushed at the end of
          //   * ogg_writeHeaderPage
          //   * ogg_writeAudioDataPage
          //   * ogg_writeFooterPage

          ogg_begin(&oggzBuffer);
          ogg_writeHeaderPage(cfg.channels, cfg.sample_rate);
#endif
          initialized = true;
        } else {
          if (DEBUG_LOG) printf("Double initialize\n˝");
        }
        com_param->result.exec_result = Wien2::Apu::ApuExecOK;

      } else if (com_param->header.process_mode == Wien2::Apu::EncMode &&
                 com_param->header.event_type == Wien2::Apu::ExecEvent) {
        // Unpack the exec command
        Wien2::Apu::ApuExecEncCmd execCmd = com_param->exec_enc_cmd;

        if (DEBUG_LOG) printf("Process ApuExecEncCmd. Input size %d\n", execCmd.input_buffer.size);

        // check for a correct frame size. Opus encoder accepts only certain sizes.
        // In our case we only accept 20ms frames.
        if (execCmd.input_buffer.size == opusEncoder->getExpectedPackageSize()) {
          execCmd.output_buffer.size = opusEncoder->encodeFrame(
              (uint8_t *)execCmd.input_buffer.p_buffer, execCmd.input_buffer.size,
              (uint8_t *)execCmd.output_buffer.p_buffer, execCmd.output_buffer.size);

#if USE_OGG
          posInSamples +=
              (execCmd.input_buffer.size / sizeof(int16_t) / opusEncoder->config().channels);
          ogg_writeAudioDataPage((uint8_t *)execCmd.output_buffer.p_buffer,
                                 execCmd.output_buffer.size, posInSamples);

          execCmd.output_buffer.size = oggzBuffer.size();
          memcpy(execCmd.output_buffer.p_buffer, oggzBuffer.data(), oggzBuffer.size());
          oggzBuffer.clear();
#endif
          com_param->result.exec_result = Wien2::Apu::ApuExecOK;
        } else {
          if (DEBUG_LOG)
            printf("The package size for %d channel %d MHz sampling rate muste be %d\n",
                   opusEncoder->config().channels, opusEncoder->config().sample_rate,
                   opusEncoder->getExpectedPackageSize());
          execCmd.output_buffer.size = 0;
          com_param->result.exec_result = Wien2::Apu::ApuWarning;
        }
        if (DEBUG_LOG)
          printf("Process ApuExecEncCmd. Output size %d\n", execCmd.output_buffer.size);
      } else if (com_param->header.process_mode == Wien2::Apu::EncMode &&
                 com_param->header.event_type == Wien2::Apu::FlushEvent) {
        // Unpack the exec command
        Wien2::Apu::ApuFlushEncCmd flushCmd = com_param->flush_enc_cmd;

        if (DEBUG_LOG) printf("Process ApuFlushEncCmd\n");

#if USE_OGG
        ogg_writeFooterPage();
        ogg_end();
        flushCmd.output_buffer.size = oggzBuffer.size();
        memcpy(flushCmd.output_buffer.p_buffer, oggzBuffer.data(), oggzBuffer.size());
        oggzBuffer.clear();
#endif
        com_param->result.exec_result = Wien2::Apu::ApuExecOK;
        if (DEBUG_LOG)
          printf("Process ApuFlushEncCmd. Output size %d\n", flushCmd.output_buffer.size);
      }
    } else {
      if (ERROR_LOG) printf("Error. Expected type COMMAND_DATATYPE_ADDRESS but was %d.\n", type);
    }

    // Send Reply
    reply_to_spu(reinterpret_cast<void *>(msgdata));
  }
  return 0;
}

int main() {
#if INCREASED_STACK_SIZE_IN_SDK
  __main();
#else
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  attr.stacksize = 25000;
  pthread_t handleAudio_id;
  pthread_create(&handleAudio_id, &attr, (pthread_startroutine_t)__main, NULL);
  pthread_setname_np(handleAudio_id, "__main");
#endif
  return 0;
}
