#include "include.h"
#include <signal.h>

DEV_CFG_PARAM devCfgParam;

void initDefaultParam(void)
{
	if (NULL == devCfgParam.capParam.dev_name){
		devCfgParam.capParam.dev_name = "/dev/video0";
	}
	devCfgParam.capParam.width = WIDTH;
	devCfgParam.capParam.height = HEIGHT;
	devCfgParam.capParam.rate = FRAMERATE;
	devCfgParam.capParam.pixfmt = YUV_INPUT_FMT;
	
	devCfgParam.yuvParam.width = WIDTH;
	devCfgParam.yuvParam.height = HEIGHT;
	devCfgParam.yuvParam.r = 255;
	devCfgParam.yuvParam.g = 0;
	devCfgParam.yuvParam.b = 0;
	
	devCfgParam.convertParam.inwidth = WIDTH;
	devCfgParam.convertParam.inheight = HEIGHT;
	devCfgParam.convertParam.inpixfmt = V4L2_PIX_FMT_YUYV;
	devCfgParam.convertParam.outwidth= WIDTH;
	devCfgParam.convertParam.outheight = HEIGHT;
	devCfgParam.convertParam.outpixfmt = V4L2_PIX_FMT_YUV420;

	devCfgParam.encParam.src_picwidth = WIDTH;
	devCfgParam.encParam.src_picheight = HEIGHT;
	devCfgParam.encParam.enc_picwidth = WIDTH;
	devCfgParam.encParam.enc_picheight = HEIGHT;
	devCfgParam.encParam.fps = FRAMERATE;
	devCfgParam.encParam.bitrate = BIT_RATE_KBPS;
	devCfgParam.encParam.gop = GOP;
	devCfgParam.encParam.chroma_interleave = 0;
}

int running = 1;
extern void prt_soft_version(void);

void Stop(int signo)
{
	printf("interrupt: %d\n", signo);
	running = 0;
}

int main(int argc, char *argv[])
{
	prt_soft_version();
	
	signal(SIGINT, Stop);
	initDefaultParam();

	task_creat(NULL, 90, 8192, (FUNC)encode_task, NULL);
	//task_creat(NULL, 60, 2048, (FUNC)record_task, NULL);
	task_creat(NULL, 60, 2048, (FUNC)rtsp_server_task, NULL);
	encode_start();
	
	while (running) {
		sleep(10);
	}
	
	encode_exit();
	
	return 0;
}









