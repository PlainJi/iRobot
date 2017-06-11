#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include "recpcm.h"
#include "tl.h"
#include "bd.h"

extern PCM_INFO pcmInfo;
int start = -1;
int end = -1;
extern int running;

void writeFile(unsigned char *buf, int len)
{
	FILE *fp = NULL;
	static int cnt = 0;
	char temp[32] = {0};

	sprintf(temp, "record_%d.pcm", cnt++);
	
	fp = fopen(temp, "wb");
	if(NULL==fp) {
		printf("creat file failed!\n");
		return;
	}
	
	fwrite(buf, 1, len, fp);
	fflush(fp);
	fclose(fp);
	//printf("write file %s\n", temp);
}

#if 0
int get_pcm_data(unsigned char *buf, int maxlen, int start, int end)
{
	int fd = open("./test.pcm", O_RDONLY, 0666);
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	return read(fd, buf, size);
}
#else
int get_pcm_data(unsigned char *buf, int maxlen, int start, int end)
{
	int len = GET_DIS(start, end, BUF_FLAG_CNT)*(JUDGE_TIME_MSEC/MSEC_PER_TIME)*BYTES_PER_TIME;
	printf("%d-%d, len=%d\n", start, end, len);

	if(len > maxlen) {
		printf("length to big!!!\n");
		return -1;
	}

	char *pStart = pcmInfo.recordBuf + start*(JUDGE_TIME_MSEC/MSEC_PER_TIME)*BYTES_PER_TIME;
	if(start < end) {
		memcpy(buf, pStart, len);
	} else {
		int len1 = (BUF_FLAG_CNT-start)*(JUDGE_TIME_MSEC/MSEC_PER_TIME)*BYTES_PER_TIME;
		int len2 = len - len1;
		memcpy(buf, pStart, len1);
		memcpy(buf+len1, pcmInfo.recordBuf, len2);
	}

	return len;
}
#endif

int fragmentData(void)
{
	int *pw = &pcmInfo.pw;
	int *pr = &pcmInfo.pr;
	int *ps = &pcmInfo.pstart;
	int *pe = &pcmInfo.pend;
	char *bufFlag = pcmInfo.bufFlag;
	static int noVoiceCnt = 0;

	while(*pr != *pw) {
		if (-1==(*ps) && bufFlag[*pr]) {
			*ps = *pr;
			noVoiceCnt = 0;
		}
		if (!bufFlag[*pr]) {
			noVoiceCnt++;
		}
		if (-1!=(*ps) && noVoiceCnt>=2) {
			*pe = *pr;
			if(GET_DIS(*ps, *pe, BUF_FLAG_CNT)>5) {
				*ps = GET_BACKWORD_N(*ps, BUF_FLAG_CNT, 2);
				start = *ps;
				end = *pe;
			}
			*ps = -1;
			*pe = -1;
			noVoiceCnt = 0;
		}
		
		FORWORD(*pr, BUF_FLAG_CNT);
	}

	return 0;
}

int jurge(char *buffer, int len)
{
	unsigned int i = 0;
	short tmp = 0;
	static unsigned int sum = 0;
	static unsigned int cnt = 0;
	int curState = 0;
	int *pw = &pcmInfo.pw;
	
	if (8 == RECORD_SAM_BITS) {
		for(i=0; i<len; i++) {
			sum += pow(abs(buffer[i] - 0x80), 2);
		}
	} else if(16 == RECORD_SAM_BITS) {
		for(i=0; i<len; i+=2) {
			tmp = buffer[i+1]<<8 | buffer[i];
			sum += pow(abs(tmp/32), 2);
		}
	} else {
		printf("ERROR Sample bits, set to 8bit!!!\n");
	}
	
	cnt++;
	if( cnt >= (JUDGE_TIME_MSEC/MSEC_PER_TIME)) {
		if (8 == RECORD_SAM_BITS) {
			if (sum < 1000000) {
				curState = 0;
				//printf("%4d 0 %9u\n", *pw, sum);
			}else {
				curState = 1;
				//printf("%4d 1 %9u\n", *pw, sum);
			}
		} else if(16 == RECORD_SAM_BITS) {
			if(sum < 500000) {
				curState = 0;
				//printf("%4d 0 %9u\n", *pw, sum);
			} else {
				curState = 1;
				//printf("%4d 1 %9u\n", *pw, sum);
			}
		}
		sum = 0;
		cnt = 0;
		pcmInfo.bufFlag[*pw] = curState;
		FORWORD(*pw, BUF_FLAG_CNT);
		fragmentData();
	}

	return 0;
}

int talk_task(void)
{
	u32 len = 0;
	char *recResult = NULL;
	char *tl_answer = NULL;
	u8 *pcm_file_buf = NULL;
	
	if(NULL == (pcm_file_buf=(u8*)malloc(PCM_DATA_BUF))) {
		printf("talk_task malloc pcm_file_buf failed!\n");
		goto exit;
	}
	if(NULL == (recResult=(char*)malloc(VOICE_STR_LEN))) {
		printf("talk_task malloc recResult failed!\n");
		goto exit;
	}
	if(NULL == (tl_answer=(char*)malloc(VOICE_STR_LEN))) {
		printf("talk_task malloc tl_answer failed!\n");
		goto exit;
	}
	
	if(bd_init()<0) {
		goto exit;
	}
	
	while (running) {
		if(-1 != start && -1 != end) {
			if((len=get_pcm_data(pcm_file_buf, PCM_DATA_BUF, start, end)) > 0) {
				//writeFile(pcm_file_buf, len);
				if(!bd_voice_recognition((const char*)pcm_file_buf, len, 
					recResult, VOICE_STR_LEN) && strlen(recResult)) {
					if(!tl_ask(recResult, 1, tl_answer, VOICE_STR_LEN) && strlen(tl_answer)) {
						if(!bd_tts(tl_answer, 1, TTS_FILE)) {
							system("mplayer out.mp3 > /dev/null 2>&1");
						}
					}
				}
			}
			start = -1;
			end = -1;
			pcmInfo.pr = pcmInfo.pw;
		}
		usleep(40*1000);
	}

exit:
	if(pcm_file_buf) free(pcm_file_buf);
	if(recResult) free(recResult);
	if(tl_answer) free(tl_answer);
	bd_deinit();
	return 0;
}

