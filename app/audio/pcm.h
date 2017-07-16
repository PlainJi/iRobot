#ifndef __PCM_H
#define __PCM_H

#define ALSA_PCM_NEW_HW_PARAMS_API
//SND_PCM_FORMAT_U8			//8位无符号
//SND_PCM_FORMAT_S16_LE		//16位小端模式
//单声道
#define CHANNELS			(1)
//16位采样深度
#define RECORD_SAM_BITS		(16)
//每帧的字节数
#define BYTES_PER_FRAME		(RECORD_SAM_BITS/8*CHANNELS)
//8K采样率
#define RECORD_FPS			(8000)
//每次读取128帧PCM数据
#define RECORD_FRAMES		(128)
//128帧*(16/8)字节*1声道=256bytes 即一次读取256字节
#define BYTES_PER_TIME		(RECORD_FRAMES*BYTES_PER_FRAME)
//256/(8000*2*1)*1000=16ms 即每16ms读一次数据
#define MSEC_PER_TIME		(BYTES_PER_TIME*1000/(RECORD_FPS*BYTES_PER_FRAME))
//循环存储2048个读取周期的数据
#define RECORD_READ_CNT		(2048)
//2048*256 即要分配的buffer大小
#define RECORD_BUFFER_SIZE	(RECORD_READ_CNT*BYTES_PER_TIME)
//可以存储2048*16=32768ms的PCM数据
#define RECORD_TOTAL_TIME	(RECORD_READ_CNT*MSEC_PER_TIME)
//64ms判断一次是否有音频输入
#define JUDGE_TIME_MSEC		(64)
//大buffer里包含BUF_FLAG_CNT个判断单元
#define BUF_FLAG_CNT		(RECORD_READ_CNT/(JUDGE_TIME_MSEC/MSEC_PER_TIME))

#define FORWORD(a,b)		((a)=(((a)+1)%(b)))
#define BACKWORD(a,b)		((a)=((((a)-1)<0)?(b-1):((a)-1)))
#define GET_FORWORD_N(a,b,n)	(((a)+n)%(b))
#define GET_BACKWORD_N(a,b,n)	((((a)-n)<0)?(a+b-n):((a)-n))
#define GET_DIS(a,c,b)		( (c>a)?(c-a):(c+b-a) )

typedef struct
{
	char *recordBuf;
	int pwrite;
	
	char bufFlag[BUF_FLAG_CNT];
	int pw;
	int pr;
	int pstart;
	int pend;
}PCM_INFO;

#endif
