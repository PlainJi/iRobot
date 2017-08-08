#include "include.h"

typedef enum encode_stat_m
{
	ENCODE_EXIT = 0,
	ENCODE_IDLE,
	ENCODE_START,
	ENCODE_STOP,
}ENCODE_STAT;

static CAP_HANDLE *capHandle = NULL;
static YUV_HANDLE *yuvHandle = NULL;
static CONVERT_HANDLE *convertHandle = NULL;
static ENC_HANDLE *encHandle = NULL;
static ENCODE_STAT encodeState = ENCODE_IDLE;
ENCODE_BUF encodeBuf;
extern DEV_CFG_PARAM devCfgParam;

int encode_start(void)
{
	capHandle = capture_open(devCfgParam.capParam);
	if (!capHandle){
		printf("--- Open capture failed\n");
		return -1;
	}
	yuvHandle = osd_open(devCfgParam.yuvParam);
	if (!yuvHandle){
		printf("--- Open yuvOSD failed\n");
		return -1;
	}
	convertHandle = convert_open(devCfgParam.convertParam);
	if (!convertHandle){
		printf("--- Open convert failed\n");
		return -1;
	}
	encHandle = encode_open(devCfgParam.encParam);
	if(!encHandle){
		printf("--- Open encode failed\n");
		return -1;
	}
	
	capture_start(capHandle);
	encodeState = ENCODE_START;

	return 0;
}

void encode_stop(void)
{
	printf("+++ stop encode\n");
	encodeState = ENCODE_STOP;
	usleep(40000);			//wait for the last encoding frame

	encode_close(encHandle);
	convert_close(convertHandle);
	capture_stop(capHandle);
	capture_close(capHandle);
}

static void calFps(void)
{
	u64 dif;
	static u32 fps = 0;
	static u32 frameNo = 0;
	static struct timeval ctime, ltime;

	gettimeofday(&ctime, NULL);
	dif = (ctime.tv_sec*1000 + ctime.tv_usec/1000) - (ltime.tv_sec*1000 + ltime.tv_usec/1000);
	if (dif >= 1000){
		printf("FPS: %u\n", fps);
		ltime = ctime;
		fps = 0;
	}
	fps++;
	frameNo++;
}

static int initEncodeBuf(void)
{
	memset(&encodeBuf, 0, sizeof(encodeBuf));
	encodeBuf.h264Buf = (u8*)malloc(BUFFER_SIZE);
	if(NULL == encodeBuf.h264Buf) {
		perror("malloc");
		return -1;
	}
	encodeBuf.curWP = encodeBuf.h264Buf;

	return 0;
}

//include 00 00 00 01 or 00 00 01
void putOneFrame(u8 *buf, u32 len, PIC_TYPE type)
{
	u8 *p = NULL;

	if(type == 6 || type == 7 || type == 8) {
		return;
	}
	
	if((encodeBuf.written+len) > BUFFER_SIZE) {
		encodeBuf.curWP = encodeBuf.h264Buf;
		encodeBuf.written = 0;
	}
	
	p = encodeBuf.curWP;
	memcpy(p, buf, len);
	encodeBuf.written += len;
	encodeBuf.curWP = (u8*)((u32)p + len);

	encodeBuf.frameInfo[encodeBuf.writeIdx].p = p;
	encodeBuf.frameInfo[encodeBuf.writeIdx].len = len;
	encodeBuf.frameInfo[encodeBuf.writeIdx].time = time(NULL);
	encodeBuf.frameInfo[encodeBuf.writeIdx].type = (u8)type;
	//printf("put [%3d] [%d] %6d %02X %02X %02X %02X %02X\n", encodeBuf.writeIdx, type, len, 
	//			buf[0], buf[1], buf[2], buf[3], buf[4]);

	encodeBuf.totalFrames++;
	IDX_INCREASE(encodeBuf.writeIdx);
}

int saveYUVPic(char *name, u8 *p, int len)
{
	FILE *fp = fopen(name, "wb");
	if(fp == NULL) {
		return -1;
	}

	fwrite(p, 1, len, fp);

	fclose(fp);

	return 0;
}

void encode_task(void)
{
	int ret;
	void *capBuf, *cvtBuf, *encBuf;
	int capLen, cvtLen, encLen;
	PIC_TYPE picType;

	initEncodeBuf();
	
	while (ENCODE_EXIT != encodeState) {
		if (ENCODE_START != encodeState) {
			usleep(50000);
			continue;
		}
		
		calFps();

		if(0 != (ret = capture_get_data(capHandle, &capBuf, &capLen)) ) {
			if (ret < 0) {
				printf("--- capture_get_data failed\n");
				break;
			}else {
				usleep(10000);
				printf("again\n");
				continue;
			}
		}
		if (capLen <= 0) {
			printf("!!! No capture data\n");
			continue;
		}
		
		osd_render_time(yuvHandle, capBuf);

		convert_do(convertHandle, capBuf, capLen, &cvtBuf, &cvtLen);

		if ((ret = encode_one_frame(encHandle, cvtBuf, cvtLen, &encBuf, &encLen, &picType)) <0) {
			printf("--- encode_do failed\n");
			continue;
		}
		if (encLen <= 0) {
			printf("!!! No encode data, len=%d\n", encLen);
			continue;
		}

		putOneFrame(encBuf, encLen, picType);
	}
	
	encode_stop();
}

void encode_exit(void)
{
	encodeState = ENCODE_EXIT;
	sleep(1);
}

int get_one_frame(u32 idx, FRAME_INFO *frameInfo)
{
	if(idx>=FRAME_INFO_CNT) {
		frameInfo = NULL;
		return -1;
	}
	
	memcpy(frameInfo, &encodeBuf.frameInfo[idx], sizeof(FRAME_INFO));
	return 0;
}

void get_encode_buf(ENCODE_BUF **p)
{
	*p = &encodeBuf;
}

int get_last_I_frame_Idx(u32 idx)
{
	int tryTimes = 200;
	u32 curIdx = idx;

	if(curIdx>=FRAME_INFO_CNT) {
		return -1;
	}
	
	while(tryTimes--) {
		if(5 == encodeBuf.frameInfo[curIdx].type) {
			return curIdx;
		}
		IDX_DECREASE(curIdx);
	}

	return -1;
}

int check_I_frame(u32 idx)
{
	if(idx>=FRAME_INFO_CNT) {
		return -1;
	}
	
	if(5 == encodeBuf.frameInfo[idx].type) {
		return 1;
	}

	return 0;
}

u8 get_frame_type(u32 idx)
{
	if(idx>=FRAME_INFO_CNT) {
		return -1;
	}
	
	return encodeBuf.frameInfo[idx].type;
}



