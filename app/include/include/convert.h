#ifndef CONVERT_H
#define CONVERT_H

#include "comdef.h"

struct cvt_param
{
	int inwidth;	//input image width
	int inheight;	//input image height
	u32 inpixfmt;	//input image pixel format
	int outwidth;	//output image width
	int outheight;	//output image height
	u32 outpixfmt;	//output image pixel format
};

struct cvt_handle;

struct cvt_handle *convert_open(struct cvt_param param);
void convert_close(struct cvt_handle *handle);
int convert_do(struct cvt_handle *handle, const void *inbuf, int isize, void **poutbuf, int *posize);

#endif
