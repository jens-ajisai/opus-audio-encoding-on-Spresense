#include <opus/opusenc.h>
#include <stdio.h>

#define READ_SIZE_FROM_FILE 480

void doEncode(const char *filenameIn, const char *filenameOut) {
  FILE *fin;
  OggOpusEnc *enc;
  OggOpusComments *comments;
  int error;
  fin = fopen(filenameIn, "rb");
  if (!fin) {
    fprintf(stderr, "cannot open input file: %s\n", filenameIn);
    return;
  }
  comments = ope_comments_create();
  ope_comments_add(comments, "ARTIST", "Someone");
  ope_comments_add(comments, "TITLE", "Some track");
  enc = ope_encoder_create_file(filenameOut, comments, 48000, 1, 0, &error);
  if (!enc) {
    fprintf(stderr, "error encoding to file %s: %s\n", filenameOut,
            ope_strerror(error));
    ope_comments_destroy(comments);
    fclose(fin);
    return;
  }
  while (1) {
    fprintf(stdout, "read next chunk\n");
    short buf[READ_SIZE_FROM_FILE];
    int ret = fread(buf, sizeof(short), READ_SIZE_FROM_FILE, fin);
    if (ret > 0) {
      ope_encoder_write(enc, buf, ret / 2);
    } else
      break;
  }
  ope_encoder_drain(enc);
  ope_encoder_destroy(enc);
  ope_comments_destroy(comments);
  fclose(fin);
  fprintf(stdout, "finished\n");
  return;
}