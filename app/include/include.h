#ifndef INCLUDE_H
#define INCLUDE_H

#include "capture_v4l.h"
#include "osd_yuyv.h"
#include "convert_yuyv.h"
#include "encode_omx.h"
#include "encode_main.h"

#include "mp4.h"
#include "PS.h"
#include "record.h"

#include "sock.h"
#include "rtp.h"
#include "rtspServer.h"
#include "posix.h"

#include "bd.h"
#include "tl.h"

//#define WIDTH 160
//#define HEIGHT 120

//#define WIDTH 320
//#define HEIGHT 240

//#define WIDTH 640
//#define HEIGHT 480

//#define WIDTH 800
//#define HEIGHT 600

//#define WIDTH 1024
//#define HEIGHT 768

#define WIDTH 1280
#define HEIGHT 720

//#define WIDTH 1920
//#define HEIGHT 1080

#define YUV_INPUT_FMT	V4L2_PIX_FMT_YUYV
#define YUV_OUTPUT_FMT	V4L2_PIX_FMT_YUV420
#define FRAMERATE 		25
#define GOP				20
#define BIT_RATE_KBPS	2000

typedef struct dev_cfg_param_t
{
	CAP_PARAM capParam;
	YUV_PARAM yuvParam;
	CONVERT_PARAM convertParam;
	ENC_PARAM encParam;
	RECORD_PARAM recordParam;
	RTSP_PARAM rtspParam;
}DEV_CFG_PARAM;

#endif









