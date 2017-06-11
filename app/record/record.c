#include "include.h"

static RECORD_INFO recordInfo;
extern DEV_CFG_PARAM devCfgParam;

int switchFile(u32 *rIdx)
{
	int outputFD = 0;
	u8 *sps = NULL;
	u8 *pps = NULL;
	u8 time[20] = {0};
	char fileName[64] = {0};
	u32 spsLen = 0, ppsLen = 0, iFrameIdx = 0;

	if(recordInfo.fileSize > devCfgParam.recordParam.sizePerFile) {
		printf("############## Finalize Record File [%d] #############\n", recordInfo.fileNo);
		closeMP4();
		recordInfo.fileSize = 0;
	}

	if(!recordInfo.fileSize) {
		getShortDateTimeStr(time);
		sprintf((char*)fileName, "%s%s_%02d.mp4", devCfgParam.recordParam.recordPath, time, recordInfo.fileNo++);
		openMP4(fileName);

		while(!get_spspps_state()) {
			sleep(1);
		}
		get_SPS(&sps, &spsLen);
		get_PPS(&pps, &ppsLen);
		writeMP4(sps, spsLen, 7);
		writeMP4(pps, ppsLen, 8);
		printf("############## Creat Record File [%d] #############\n", recordInfo.fileNo);

		iFrameIdx = get_last_I_frame_Idx(*rIdx);
		if(iFrameIdx >= 0) {
			*rIdx = iFrameIdx;
		}
	}

	return 0;
}

int writeRecodeFile(u8 *buf, u32 len, u8 type)
{
	if(7==type || 8==type || 6==type) {
		return 0;
	}

	writeMP4((const u8*)buf, len, type);
	recordInfo.fileSize += len;

	return 0;
}

void record_task(void)
{
	int fileSize = 0;
	FRAME_INFO frameInfo;
	ENCODE_BUF *encodeBuf = NULL;
	volatile u32 *wIdx = NULL;
	u32 *rIdx = NULL;
	
	memset(&recordInfo, 0, sizeof(recordInfo));
	get_encode_buf(&encodeBuf);
	wIdx = &encodeBuf->writeIdx;
	rIdx = &encodeBuf->readIdxRecord;
	
	while(1) {
		if(*wIdx == *rIdx) {
			usleep(40000);
			continue;
		}
		
		get_one_frame(*rIdx, &frameInfo);
		switchFile(rIdx);
		writeRecodeFile(frameInfo.p, frameInfo.len, frameInfo.type);
		IDX_INCREASE(*rIdx);
	}

	closeMP4();
}

