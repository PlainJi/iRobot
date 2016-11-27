#ifndef YUVOSD_H
#define YUVOSD_H

#include "comdef.h"

struct yuv_param
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char y;
	unsigned char u;
	unsigned char v;
	unsigned char res[2];
	unsigned int width;
	unsigned int height;
};

struct yuv_handle
{
	unsigned char *buf;
	struct yuv_param params;
};

struct yuv_handle *yuvOSD_open(struct yuv_param param);
void drawSquare(struct yuv_handle *yuvHandle, unsigned int xx, unsigned int yy, unsigned int width, unsigned int height, unsigned char *buf);
void getShortDateTimeStr(unsigned char *timeTab);
void OSDTime_do(struct yuv_handle *yuvHandle, unsigned char *buf);

#endif