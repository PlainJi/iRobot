#ifndef OSD_YUYV_H
#define OSD_YUYV_H

#include "comdef.h"

typedef struct yuv_param_t
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
}YUV_PARAM;

typedef struct yuv_handle_t
{
	unsigned char *buf;
	YUV_PARAM params;
}YUV_HANDLE;

void drawSquare(YUV_HANDLE *yuvHandle, u32 xx, u32 yy, u32 width, u32 height, u8 *buf);
void getShortDateTimeStr(u8 *timeTab);

YUV_HANDLE *osd_open(YUV_PARAM param);
void osd_render_time(YUV_HANDLE *yuvHandle, u8 *buf);

#endif