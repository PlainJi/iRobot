#ifndef CONVERT_YUYV_H
#define CONVERT_YUYV_H

#include "comdef.h"
#include <linux/videodev2.h>

typedef struct cvt_param_t
{
	int inwidth;	//input image width
	int inheight;	//input image height
	u32 inpixfmt;	//input image pixel format
	int outwidth;	//output image width
	int outheight;	//output image height
	u32 outpixfmt;	//output image pixel format
}CONVERT_PARAM;


typedef struct cvt_handle_t
{
	int src_buffersize;
	u8 *dst_buffer;
	int dst_buffersize;
	CONVERT_PARAM params;
}CONVERT_HANDLE;

CONVERT_HANDLE *convert_open(CONVERT_PARAM param);
void convert_close(CONVERT_HANDLE *handle);
int convert_do(CONVERT_HANDLE *handle, const void *inbuf, int isize, void **poutbuf, int *posize);

#endif
