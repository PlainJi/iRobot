#ifndef CAPTURE_H
#define CAPTURE_H

#include "comdef.h"

struct cap_param
{
	char *dev_name;	//video device location, eg: /dev/video0
	int width;		//video width
	int height;		//video height
	u32 pixfmt;		//video pixel format
	int rate;		//video rate
};

struct cap_handle;

struct cap_handle *capture_open(struct cap_param param);
void capture_close(struct cap_handle *handle);
int capture_start(struct cap_handle *handle);
void capture_stop(struct cap_handle *handle);
int capture_get_data(struct cap_handle *handle, void **pbuf, int *plen);

int capture_query_brightness(struct cap_handle *handle, int *min, int *max, int *step);
int capture_get_brightness(struct cap_handle *handle, int *val);
int capture_set_brightness(struct cap_handle *handle, int val);

int capture_query_contrast(struct cap_handle *handle, int *min, int *max, int *step);
int capture_get_contrast(struct cap_handle *handle, int *val);
int capture_set_contrast(struct cap_handle *handle, int val);

int capture_query_saturation(struct cap_handle *handle, int *min, int *max, int *step);
int capture_get_saturation(struct cap_handle *handle, int *val);
int capture_set_saturation(struct cap_handle *handle, int val);

#endif
