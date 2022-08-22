#pragma once

void beginRecorder(int recoding_cannel_number, int recoding_sampling_rate,
                   int recoding_bit_length, int analog_mic_gain);
void endRecorder();

size_t getAudio(uint8_t *buffer, size_t length);