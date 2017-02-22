/*
 *	��ʱֻ֧�ֵ�����
 *	������8K��16K
 *	��������8λ��16λ
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
//SND_PCM_FORMAT_U8			//8λ�޷���
//SND_PCM_FORMAT_S16_LE		//16λС��ģʽ
#define RECORD_SAM_BITS		(16)
#define RECORD_FPS			(8000)
#define JUDGE_TIME			(500)		//ms
#define RECORD_FRAMES		(128)
#define RECORD_BUFFER_SIZE	(RECORD_FRAMES*RECORD_SAM_BITS/8*1024)

char *recordBuf = NULL;
int cp = 0;
int speekTimes = 0;
int stop = 0;
int start = 0;
extern int running;

void saveData(char *buf, int len)
{
	memcpy(recordBuf+cp, buf, len);
	cp += len;
	if(cp >= RECORD_BUFFER_SIZE) {
		cp = 0;
	}
}

int jurge(char *buffer, int len)
{
	unsigned int i = 0;
	short tmp = 0;
	static unsigned int sum = 0;
	static unsigned int cnt = 0;
	static int lastState = 0;
	int curState = 0;
	
	if(8 == RECORD_SAM_BITS) {
		for(i=0; i<len; i++) {
			sum += pow(abs(buffer[i] - 0x80), 2);
		}
	} else if(16 == RECORD_SAM_BITS) {
		for(i=0; i<len; i+=2){
			tmp = buffer[i+1]<<8 | buffer[i];
			sum += pow(abs(tmp/32), 2);
		}
	} else {
		printf("ERROR Sample bits, set to 8bit!!!\n");
	}
	
	cnt++;
	if( cnt > JUDGE_TIME *  (RECORD_FPS / RECORD_FRAMES) / 1000) {
		if(8 == RECORD_SAM_BITS) {
			if(sum < 1000000) {
				stop++;
				curState = 0;
				printf("%09u, stop, ", sum);
			}else {
				start++;
				curState = 1;
				printf("%09u, start, ", sum);
			}
		} else if(16 == RECORD_SAM_BITS) {
			if(sum < 3000000) {
				stop++;
				curState = 0;
				printf("%09u, stop, ", sum);
			} else {
				start++;
				curState = 1;
				printf("%09u, start, ", sum);
			}
		}
		cnt = 0;
		sum = 0;
		
		if(!start || stop>2) {
			start = 0;
			printf("stop speek! ");
		}
		if(curState || (!curState && start>0 && stop<=2)) {
			stop = 0;
			printf("speeking... ");
		}
		if(curState && stop<2) {
			start += stop;
			stop = 0;
		}
		
		printf("curState=%d, lastState=%d, start=%d, stop=%d\n", 
			curState, lastState, start, stop);
		lastState = curState;
	}

	return 0;
}

int rec_pcm(void)
{
	unsigned long loops;
	int rc;
	int size;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;
	snd_pcm_uframes_t frames;
	char *buffer;
	
	//���豸
	rc = snd_pcm_open(&handle, "plughw:1,0", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}
	//Ϊ���ò�������ռ�
	snd_pcm_hw_params_alloca(&params);
	//Ĭ�ϲ���
	snd_pcm_hw_params_any(handle, params);
	//�ǽ�������ģʽ
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_NONINTERLEAVED);	//SND_PCM_ACCESS_RW_INTERLEAVED
	//��������
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
	//������
	snd_pcm_hw_params_set_channels(handle, params, 1);
	//����Ƶ��
	val = RECORD_FPS;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
	//ÿ����128֡����
	frames = RECORD_FRAMES;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
	//���ò���
	if(snd_pcm_hw_params(handle, params)<0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		exit(1);
	}

	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	size = frames * 1 * RECORD_SAM_BITS/8;			//1������
	buffer = (char *) malloc(size);
	recordBuf = (char *) malloc(RECORD_BUFFER_SIZE);
	snd_pcm_hw_params_get_period_time(params, &val, &dir);

	while (running) {
		rc = snd_pcm_readn(handle, (void**)&buffer, frames);
		if (rc == -EPIPE) {		//���ݻ���������
			fprintf(stderr, "overrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
		} else if (rc != (int)frames) {
			fprintf(stderr, "short read, read %d frames\n", rc);
		}
		saveData(buffer, size);
		//����ƽ��ֵ
		jurge(buffer, size);
	}
	
	if(buffer)
		free(buffer);
	if(recordBuf)
		free(recordBuf);
	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	return 0;
}  