#ifndef ENCODE_H
#define ENCODE_H

#include "comdef.h"

#define ENCODE_BUF_SIZE			(500*1024)
#define OMX_VIDENC_INPUT_PORT	200
#define OMX_VIDENC_OUTPUT_PORT	201

enum pic_t
{
	FRAME_TYPE_NONE = -1,
	FRAME_TYPE_P =1,
	FRAME_TYPE_I = 5,
	FRAME_TYPE_SEI = 6,
	FRAME_TYPE_SPS = 7,
	FRAME_TYPE_PPS = 8,
};

struct enc_param
{
	int src_picwidth;	//the source picture width
	int src_picheight;	//the source picture height
	int enc_picwidth;	//the encoded picture width
	int enc_picheight;	//the encode picture height
	int fps;			//frames per second
	int bitrate;		//bit rate(kbps), set bit rate to 0 means no rate control, the rate will depend on QP
	int gop;			//the size of group of pictures
	int chroma_interleave;//whether chroma interleaved
};

struct enc_buf
{
	char *buf;
	int len;
};

struct enc_SPSPPS
{
	unsigned int  init;
	unsigned char privateSPS[64];
	unsigned int  spsLen;
	unsigned char privatePPS[64];
	unsigned int  ppsLen;
};

struct enc_handle;
struct enc_handle *encode_open(struct enc_param param);
void encode_close(struct enc_handle *handle);
int encode_get_headers(struct enc_handle *handle, void **pbuf, int *plen, enum pic_t *type);
int encode_do(struct enc_handle *handle, void *ibuf, int ilen, void **pobuf, int *polen, unsigned char *type);
int encode_set_qp(struct enc_handle *handle, int val);
int encode_set_gop(struct enc_handle *handle, int val);
int encode_set_bitrate(struct enc_handle *handle, int val);
int encode_set_framerate(struct enc_handle *handle, int val);
void encode_force_Ipic(void);

#endif
