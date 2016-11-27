#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include "include.h"

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

#define FRAMERATE 		25
#define GOP				20
#define BIT_RATE_KBPS	2000
#define SIZE_PER_FILE	(250*1024*1024)

int running = 1;
extern void prt_soft_version(void);

void Stop(int signo)
{
	printf("interrupt: %d\n", signo);
	running = 0;
}

int recordFileManager(pData_Info_s frameInfo, unsigned char ptype)
{
	int outputFD = 0;
	static int fileNo = 0;
	unsigned char time[20] = {0};
	char fileName[64] = {0};
	extern struct enc_SPSPPS encSPSPPS;

	if(frameInfo->fileSize > SIZE_PER_FILE)
	{
		printf("############## Finalize Record File [%d] #############\n", fileNo);
		closeMP4();
		frameInfo->fileSize = 0;
	}
	
	if(!frameInfo->fileSize && 5==ptype)
	{
		getShortDateTimeStr(time);
		sprintf((char*)fileName, "/mnt/nfs7/%s_%02d.mp4", time, fileNo++);
		openMP4(fileName);
		writeMP4((const unsigned char*)encSPSPPS.privateSPS, encSPSPPS.spsLen, 7);
		writeMP4((const unsigned char*)encSPSPPS.privatePPS, encSPSPPS.ppsLen, 8);
		printf("############## Creat Record File [%d] #############\n", fileNo);
	}

	return 0;
}

int writeRecodeFile(pData_Info_s frameInfo, unsigned char ptype, char *enc_buf, int enc_len)
{
	if(recordFileManager(frameInfo, ptype)<0)
	{
		printf("--- creat Record File failed!\n");
	}
	
	if(0==frameInfo->fileSize && 5!=ptype)
	{
		printf("start of MP4 file, not I frame.\n");
		return 0;
	}

	if(7==ptype || 8==ptype || 6==ptype)
	{
		return 0;
	}

	writeMP4((const unsigned char*)enc_buf, enc_len, ptype);
	frameInfo->fileSize += enc_len;

	return 0;
}

int main(int argc, char *argv[])
{
	prt_soft_version();
	if (argc != 1)
	{
		printf("Usage: %s file.mp4\n", argv[0]);
		return  -1;
	}
	
	signal(SIGINT, Stop);
	
	struct cap_handle *caphandle = NULL;
	struct yuv_handle *yuvHandle = NULL;
	struct cvt_handle *cvthandle = NULL;
	struct enc_handle *enchandle = NULL;
	struct pac_handle *pachandle = NULL;
	struct net_handle *nethandle = NULL;
	struct cap_param capp;
	struct yuv_param yuvp;
	struct cvt_param cvtp;
	struct enc_param encp;
	struct pac_param pacp;
	struct net_param netp;
	Data_Info_s frameInfo;

	// set paraments
	u32 vfmt = V4L2_PIX_FMT_YUYV;
	u32 ofmt = V4L2_PIX_FMT_YUV420;

	CLEAR(capp);
	CLEAR(yuvp);
	CLEAR(cvtp);
	CLEAR(encp);
	CLEAR(pacp);
	CLEAR(frameInfo);

	capp.dev_name = "/dev/video0";
	capp.width = WIDTH;
	capp.height = HEIGHT;
	capp.pixfmt = vfmt;
	capp.rate = FRAMERATE;

	yuvp.r = 255;
	yuvp.g = 0;
	yuvp.b = 0;
	yuvp.width = WIDTH;
	yuvp.height = HEIGHT;
	
	cvtp.inwidth = WIDTH;
	cvtp.inheight = HEIGHT;
	cvtp.inpixfmt = vfmt;
	cvtp.outwidth = WIDTH;
	cvtp.outheight = HEIGHT;
	cvtp.outpixfmt = ofmt;

	encp.src_picwidth = WIDTH;
	encp.src_picheight = HEIGHT;
	encp.enc_picwidth = WIDTH;
	encp.enc_picheight = HEIGHT;
	encp.chroma_interleave = 0;
	encp.fps = FRAMERATE;
	encp.gop = GOP;
	encp.bitrate = BIT_RATE_KBPS;
	
	pacp.max_pkt_len = 1400;
	pacp.ssrc = 0x11223344;

	caphandle = capture_open(capp);
	if (!caphandle)
	{
		printf("--- Open capture failed\n");
		return -1;
	}

	yuvHandle= yuvOSD_open(yuvp);
	if (!yuvHandle)
	{
		printf("--- Open yuvOSD failed\n");
		return -1;
	}

	cvthandle = convert_open(cvtp);
	if (!cvthandle)
	{
		printf("--- Open convert failed\n");
		return -1;
	}

	enchandle = encode_open(encp);
	if (!enchandle)
	{
		printf("--- Open encode failed\n");
		return -1;
	}
	
	pachandle = pack_open(pacp);
	if (!pachandle)
	{
		printf("--- Open pack failed\n");
		return -1;
	}

	nethandle = net_open(netp);
	if (!nethandle)
	{
		printf("--- Open network failed\n");
		return -1;
	}
	
	//output
	frameInfo.nFrameType = 0;	//video
	frameInfo.u32Ssrc = 0x11223344;
	frameInfo.s64CurPts = pachandle->ts_start_millisec * 90;
	
	// start stream loop
	int ret;
	void *cap_buf, *cvt_buf, *hd_buf, *enc_buf;
	char *pac_buf = (char *) malloc(MAX_RTP_SIZE);
	int cap_len, cvt_len, hd_len, enc_len, pac_len;
	unsigned char ptype;
	
	struct timeval ctime, ltime;
	unsigned long fps_counter = 0;
	unsigned long frameNo = 0;
	int sec, usec;
	double stat_time = 0;
	
	gettimeofday(&ltime, NULL);

	capture_start(caphandle);

	while (running)
	{
		{
			gettimeofday(&ctime, NULL);
			sec = ctime.tv_sec - ltime.tv_sec;
			usec = ctime.tv_usec - ltime.tv_usec;
			if (usec < 0)
			{
				sec--;
				usec = usec + 1000000;
			}
			stat_time = (sec * 1000000) + usec;		// diff in microsecond

			if (stat_time >= 1000000)    // >= 1s
			{
				printf("*** FPS: %ld\n\n", fps_counter);

				fps_counter = 0;
				ltime = ctime;
			}
			fps_counter++;
			frameNo++;
		}
		
		ret = capture_get_data(caphandle, &cap_buf, &cap_len);
		if (ret != 0)
		{
			if (ret < 0)		// error
			{
				printf("--- capture_get_data failed\n");
				break;
			}
			else	// again
			{
				usleep(10000);
				continue;
			}
		}
		//cap_len  = WIDTH * HEIGHT * 16 / 8;
		if (cap_len <= 0)
		{
			printf("!!! No capture data\n");
			continue;
		}

		OSDTime_do(yuvHandle, cap_buf);

		ret = convert_do(cvthandle, cap_buf, cap_len, &cvt_buf, &cvt_len);
		if (ret < 0)
		{
			printf("--- convert_do failed\n");
			break;
		}
		if (cvt_len <= 0)
		{
			printf("!!! No convert data\n");
			continue;
		}

		ret = encode_do(enchandle, cvt_buf, cvt_len, &enc_buf, &enc_len, &ptype);
		if (ret < 0)
		{
			printf("--- encode_do failed\n");
			break;
		}
	 	if (enc_len <= 0)
		{
			printf("!!! No encode data\n");
			continue;
		}
		
		pack_put(pachandle, enc_buf, enc_len);
		if(getClientNum())
		{
			while (pack_get(pachandle, pac_buf, MAX_RTP_SIZE, &pac_len) == 1)
			{
				net_send(nethandle, pac_buf, pac_len, ptype);
			}
		}
		writeRecodeFile(&frameInfo, ptype, enc_buf, enc_len);
	}

	printf("exit!\n");
	capture_stop(caphandle);
	pack_close(pachandle);
	encode_close(enchandle);
	convert_close(cvthandle);
	capture_close(caphandle);
	closeMP4();

	return 0;
}
