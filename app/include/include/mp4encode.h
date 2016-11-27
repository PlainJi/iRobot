#ifndef MP4ENCODE_H
#define MP4ENCODE_H

#include "../mp4v2/mp4v2.h"

int openMP4(char *name);
int writeMP4(const unsigned char *pData, int size, int type);
int closeMP4();

#endif