#ifndef INCLUDE_H
#define INCLUDE_H

#include "include/capture_v4l.h"
#include "include/osd_yuyv.h"
#include "include/convert_yuyv.h"
#include "include/encode_omx.h"
#include "include/encode_main.h"

#include "include/mp4.h"
#include "include/PS.h"
#include "include/record.h"

#include "include/sock.h"
#include "include/rtp.h"
#include "include/rtspServer.h"
#include "posix.h"

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
}DEV_CFG_PARAM;

#endif









