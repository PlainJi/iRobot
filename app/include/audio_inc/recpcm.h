#ifndef __PCM_H
#define __PCM_H

#define ALSA_PCM_NEW_HW_PARAMS_API
//SND_PCM_FORMAT_U8			//8λ�޷���
//SND_PCM_FORMAT_S16_LE		//16λС��ģʽ
//������
#define CHANNELS			(1)
//16λ�������
#define RECORD_SAM_BITS		(16)
//ÿ֡���ֽ���
#define BYTES_PER_FRAME		(RECORD_SAM_BITS/8*CHANNELS)
//8K������
#define RECORD_FPS			(8000)
//ÿ�ζ�ȡ128֡PCM����
#define RECORD_FRAMES		(128)
//128֡*(16/8)�ֽ�*1����=256bytes ��һ�ζ�ȡ256�ֽ�
#define BYTES_PER_TIME		(RECORD_FRAMES*BYTES_PER_FRAME)
//256/(8000*2*1)*1000=16ms ��ÿ16ms��һ������
#define MSEC_PER_TIME		(BYTES_PER_TIME*1000/(RECORD_FPS*BYTES_PER_FRAME))
//ѭ���洢2048����ȡ���ڵ�����
#define RECORD_READ_CNT		(2048)
//2048*256 ��Ҫ�����buffer��С
#define RECORD_BUFFER_SIZE	(RECORD_READ_CNT*BYTES_PER_TIME)
//���Դ洢2048*16=32768ms��PCM����
#define RECORD_TOTAL_TIME	(RECORD_READ_CNT*MSEC_PER_TIME)
//64ms�ж�һ���Ƿ�����Ƶ����
#define JUDGE_TIME_MSEC		(64)
//��buffer�����BUF_FLAG_CNT���жϵ�Ԫ
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
