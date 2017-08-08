/*
 *	暂时只支持单声道
 *	采样率8K、16K
 *	采样精度8位、16位
 *	百度语音识别支持采样率为8K/16K、16bit位深的单声道
 *	百度语音识别支持的压缩格式为pcm、wav、opus、speex、amr、x-flac
*/

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "pcm.h"

static snd_pcm_t *handle = NULL;
PCM_INFO pcmInfo;
extern int running;
extern int jurge(char *buffer, int len);

void saveData(char *buf, int len)
{
	int *pw = &pcmInfo.pwrite;

	if(len!=BYTES_PER_TIME) {
		printf("error len=%d, BYTES_PER_TIME=%d\n", len, BYTES_PER_TIME);
		return;
	}
	
	memcpy(pcmInfo.recordBuf+(*pw)*BYTES_PER_TIME, buf, len);
	FORWORD(*pw, RECORD_READ_CNT);
}

int pcm_open(void)
{
	int ret = 0;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames = RECORD_FRAMES;
	unsigned int val;
	int dir;
	memset(&pcmInfo, 0, sizeof(pcmInfo));
	pcmInfo.pw = -1;
	pcmInfo.pr = -1;
	
	//打开设备
	ret = snd_pcm_open(&handle, "plughw:1,0", SND_PCM_STREAM_CAPTURE, 0);
	if (ret < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(ret));
		return -1;
	}
	//为配置参数分配空间
	snd_pcm_hw_params_alloca(&params);
	//默认参数
	snd_pcm_hw_params_any(handle, params);
	//非交叉数据模式
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_NONINTERLEAVED);	//SND_PCM_ACCESS_RW_INTERLEAVED
	//采样精度
	switch(RECORD_SAM_BITS){
		case 8:
			snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
			break;
		case 16:
			snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
			break;
		default:
			snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
			printf("ERROR Sample bits, set to 8bit!!!\n");
			break;
	}
	//单声道
	snd_pcm_hw_params_set_channels(handle, params, 1);
	//采样频率
	val = RECORD_FPS;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
	//每周期128帧数据
	frames = RECORD_FRAMES;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
	//设置参数
	if(snd_pcm_hw_params(handle, params)<0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(ret));
		return -2;
	}

	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	snd_pcm_hw_params_get_period_time(params, &val, &dir);

	printf("+++ PCM open\n");
	return 0;
}

int pcm_close(void)
{
	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	printf("+++ PCM closed\n");
	return 0;
}

int pcm_task(void)
{
	int ret = 0;
	char *tmpBuf = NULL;
	snd_pcm_uframes_t frames = RECORD_FRAMES;

	if(pcm_open()) {
		printf("!!! PCM open failed!\n");
		return -1;
	}
	
	tmpBuf = (char *) malloc(BYTES_PER_TIME);
	pcmInfo.recordBuf = (char *) malloc(RECORD_BUFFER_SIZE);
	memset(pcmInfo.recordBuf, 0, RECORD_BUFFER_SIZE);
	
	while (running) {
		ret = snd_pcm_readn(handle, (void**)&tmpBuf, frames);
		if (ret == -EPIPE) {		//数据缓冲区覆盖
			fprintf(stderr, "overrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (ret < 0) {
			fprintf(stderr, "error from read: %s\n", snd_strerror(ret));
		} else if (ret != (int)frames) {
			fprintf(stderr, "short read, read %d frames\n", ret);
		}
		saveData(tmpBuf, BYTES_PER_TIME);
		jurge(tmpBuf, BYTES_PER_TIME);
	}

	if(tmpBuf) free(tmpBuf);
	if(pcmInfo.recordBuf) free(pcmInfo.recordBuf);
	pcm_close();
	return 0;
}






