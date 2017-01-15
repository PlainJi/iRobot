#ifndef CAPTURE_V4L_H
#define CAPTURE_V4L_H

#include "comdef.h"
#include <linux/videodev2.h>

#define VIDEO_CROP_CAP

typedef struct buffer_t
{
	void *start;
	int length;
}BUFFER;

typedef struct cap_param
{
	char *dev_name;	//video device location, eg: /dev/video0
	int width;		//video width
	int height;		//video height
	int rate;		//video rate
	u32 pixfmt;		//video pixel format
}CAP_PARAM;

typedef struct cap_handle
{
	int fd;
	int image_size;
	u32 image_counter;
	BUFFER *buffers;
	u32 nbuffers;
	CAP_PARAM params;
	struct v4l2_buffer v4lbuf;	// v4l2 buffer get/put
	int v4lbuf_put;				// 0/1
}CAP_HANDLE;

CAP_HANDLE *capture_open(CAP_PARAM param);
void capture_close(CAP_HANDLE *handle);
int capture_start(CAP_HANDLE *handle);
void capture_stop(CAP_HANDLE *handle);
int capture_get_data(CAP_HANDLE *handle, void **pbuf, int *plen);

int capture_query_brightness(CAP_HANDLE *handle, int *min, int *max, int *step);
int capture_get_brightness(CAP_HANDLE *handle, int *val);
int capture_set_brightness(CAP_HANDLE *handle, int val);

int capture_query_contrast(CAP_HANDLE *handle, int *min, int *max, int *step);
int capture_get_contrast(CAP_HANDLE *handle, int *val);
int capture_set_contrast(CAP_HANDLE *handle, int val);

int capture_query_saturation(CAP_HANDLE *handle, int *min, int *max, int *step);
int capture_get_saturation(CAP_HANDLE *handle, int *val);
int capture_set_saturation(CAP_HANDLE *handle, int val);

#endif