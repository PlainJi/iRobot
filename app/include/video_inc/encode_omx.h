#ifndef ENCODE_OMX_H
#define ENCODE_OMX_H

#include "comdef.h"
#include <bcm_host.h>
#include <ilclient.h>

#undef	ENABLE_INLINE_HEADER 
#define ENCODE_BUF_SIZE			(500*1024)
#define OMX_VIDENC_INPUT_PORT	200
#define OMX_VIDENC_OUTPUT_PORT	201

typedef enum pic_m
{
	FRAME_TYPE_NONE = -1,
	FRAME_TYPE_P =1,
	FRAME_TYPE_I = 5,
	FRAME_TYPE_SEI = 6,
	FRAME_TYPE_SPS = 7,
	FRAME_TYPE_PPS = 8,
}PIC_TYPE;

typedef struct enc_spspps_t
{
	unsigned int  init;
	unsigned char privateSPS[64];
	unsigned int  spsLen;
	unsigned char privatePPS[64];
	unsigned int  ppsLen;
}ENC_SPSPPS;

typedef struct enc_param_t
{
	int src_picwidth;	//the source picture width
	int src_picheight;	//the source picture height
	int enc_picwidth;	//the encoded picture width
	int enc_picheight;	//the encode picture height
	int fps;			//frames per second
	int bitrate;		//bit rate(kbps), set bit rate to 0 means no rate control, the rate will depend on QP
	int gop;			//the size of group of pictures
	int chroma_interleave;//whether chroma interleaved
}ENC_PARAM;

typedef struct enc_buf_t
{
	char *buf;
	int len;
}ENC_BUF;

typedef struct enc_handle_t
{
	COMPONENT_T *video_encode;
	COMPONENT_T *list[5];
	ILCLIENT_T *client;
	OMX_BUFFERHEADERTYPE *out;
	u32 frame_counter;

	ENC_PARAM params;
	ENC_BUF EncodeBuf;
}ENC_HANDLE;

ENC_HANDLE *encode_open(ENC_PARAM param);
void encode_close(ENC_HANDLE *handle);
int encode_one_frame(ENC_HANDLE *handle, void *ibuf, int ilen, void **pobuf, int *polen, PIC_TYPE *type);
void encode_force_I_Frame(void);
int get_SPS(u8 **p, u32 *len);
int get_PPS(u8 **p, u32 *len);
int get_spspps_state();

//int encode_set_qp(ENC_HANDLE *handle, int val);
//int encode_set_gop(ENC_HANDLE *handle, int val);
//int encode_set_bitrate(ENC_HANDLE *handle, int val);
//int encode_set_framerate(ENC_HANDLE *handle, int val);
//int encode_get_headers(ENC_HANDLE *handle, void **pbuf, int *plen, PIC_TYPE *type);

#endif
