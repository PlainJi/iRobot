#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <netinet/in.h>
#include "include.h"

#pragma pack()
#pragma pack(1)

extern struct enc_SPSPPS encSPSPPS;
extern unsigned int mpeg_crc32(const unsigned char *data, int len);

int write_MP4_Head(pData_Info_s frameInfo)
{
	unsigned char data1[34] = {
	0x00, 0x00, 0x01, 0xBA, 0x6F, 0xB9, 0x36, 0xA0, 0xE4, 0x01, 0x32, 0x4F, 0x83, 0xF8, 0x00, 0x00, 
	0x01, 0xBC, 0x00, 0x0E, 0xE0, 0xFF, 0x00, 0x00, 0x00, 0x04, 0x1B, 0xE0, 0x00, 0x00, 0x45, 0xBD, 
	0xDC, 0xF4};
	unsigned char data2[14] = {
	0x00, 0x00, 0x01, 0xE0, 0x00, 0x1A, 0x8C, 0x80, 0x05, 0x3B, 0xF7, 0x4D, 0xA8, 0x39};
	unsigned char data3[9] = {
	0x00, 0x00, 0x01, 0xE0, 0x00, 0x0C, 0x8C, 0x00, 0x00};

	write(frameInfo->sockfd, data1, sizeof(data1));
	write(frameInfo->sockfd, data2, sizeof(data2));
	write(frameInfo->sockfd, encSPSPPS.privateSPS, encSPSPPS.spsLen);
	write(frameInfo->sockfd, data3, sizeof(data3));
	write(frameInfo->sockfd, encSPSPPS.privatePPS, encSPSPPS.ppsLen);
	frameInfo->fileSize = sizeof(data1)+sizeof(data2)+sizeof(data3)+encSPSPPS.spsLen+encSPSPPS.ppsLen;
	frameInfo->newFileFlag = 1;
	
	return 0;
}

/***
 *@remark:   ps头的封装,里面的具体数据的填写已经占位，可以参考标准
 *@param :   pData  [in] 填充ps头数据的地址
 *           s64Src [in] 时间戳
 *@return:   0 success, others failed
*/
int gb28181_make_ps_header(pPS_HEADER psHeader, unsigned long long s64Scr)
{
	memset(psHeader, 0, PS_HDR_LEN);
	
	/* byte 0-3 */
	psHeader->startcode = htonl(0x000001ba);/* start codes */
	/* byte 4-8 */
	psHeader->mark1 = 1;					/* marker bit '01b' */
	psHeader->scr1 = (s64Scr>>30)&0x7;		/* System Clock Reference [32..30] */
	psHeader->mark2 = 1;					/* marker bit */
	psHeader->scr2 = (s64Scr>>28)&0x3;		/* System Clock Reference [29..28] */
	
	psHeader->scr3 = (s64Scr>>20)&0xff;		/* System Clock Reference [27..20] */
	psHeader->scr4 = (s64Scr>>15)&0x1f;		/* System Clock Reference [19..15] */
	psHeader->mark3 = 1;					/* marker bit */
	psHeader->scr5 = (s64Scr>>13)&0x3;		/* System Clock Reference [14..13] */
	psHeader->scr6 = (s64Scr>>5)&0xff;		/* System Clock Reference [12..5] */
	psHeader->scr7 = (s64Scr>>0)&0x1f;		/* System Clock Reference [4..0] */
	psHeader->mark4 = 1;					/* marker bit */
	psHeader->exScr1 = 0;					/* SCR extension [8..7] */
	/* byte 9 */
	psHeader->exScr2 = 0;					/* SCR extension [6..0] */
	psHeader->mark5 = 1;					/* marker bit */
	/* byte 10-12 */						//unsigned int muxRate = 824288;
	psHeader->mulRate1 = 0x32;				/* multiplex rate [21.14] */
	psHeader->mulRate2 = 0x4f;				/* multiplex rate [13..6] */
	psHeader->mulRate3 = 0x20;				/* multiplex rate [5..0] */
	psHeader->mark6 = 3;					/* marker bit */
	/* byte 13 */
	psHeader->reserve = 0x1f;				/* reserved */
	psHeader->stuffing = 0;					/* stuffing */

	return 0;
}

/***
 *@remark:   sys头的封装,里面的具体数据的填写已经占位，可以参考标准
 *@param :   pData  [in] 填充ps头数据的地址
 *@return:   0 success, others failed
*/
int gb28181_make_sys_header(pSYS_HEADER sysHeader)
{
	memset(sysHeader, 0, SYS_HDR_LEN);
	
	/* byte 0-3 */
	sysHeader->startCode = htonl(0x000001BB);	/* start code */
	/* byte 4-5 */
	sysHeader->headerLen = htons(SYS_HDR_LEN-6);/* header length(not include the first 6 bytes) */
	/* byte 6-8 */
	sysHeader->mark1 = 1;						/* marker bit */
	sysHeader->rateBound1 = (50000>>15)&0x7f;	/* 21-15 max value of the multiplex rates of all packs in the stream,total 22 bits */
	sysHeader->rateBound2 = (50000>>7)&0xff;	/* 14-7 */
	sysHeader->rateBound3 = (50000>>0)&0x7f;	/* 6-0 */
	sysHeader->mark2 = 1;						/* marker bit */
	/* byte 9 */
	sysHeader->audioBound = 1;					/* max munber of audio streams in this program stream */
	sysHeader->fixedFlag = 0;					/* set for fixed bitrate */
	sysHeader->cspsFlag = 1;					/* set for constrained */
	/* byte 10 */
	sysHeader->audioLock = 1;					/* system_audio_lock_flag */
	sysHeader->videoLock = 1;					/* system_video_lock_flag */
	sysHeader->mark3 = 1;						/* marker bit */
	sysHeader->videoBound = 1;					/* max munber of video streams in this ISO stream */
	/* byte 11 */
	sysHeader->packRtRstcFlag = 1;				/* packet rate restriction flag */
	sysHeader->reserve = 0x7F;					/* set to FF */
	/* byte 12-14 */
	sysHeader->aStreamID = 0xC0;				/* audio stream ID */
	sysHeader->mark4 = 0x3;						/* marker bit */
	sysHeader->aPstdBufBndScl = 0;				/* audio P-STD buffer bound scale */
	sysHeader->aBufSizeBound1 = (512>>8)&0x1f;	/* 12-8 audio P-STD buffer size bound */
	sysHeader->aBufSizeBound2 = (512>>0)&0xff;	/* 7-0 */
	/* byte 15-17 */
	sysHeader->vStreamID = 0xE0;				/* video stream ID */
	sysHeader->mark5 = 0x3;						/* marker bit */
	sysHeader->vPstdBufBndScl = 1;				/* video P-STD buffer bound scale */
	sysHeader->vBufSizeBound1 = (2048>>8)&0x1f;	/* video P-STD buffer size bound */
	sysHeader->vBufSizeBound2 = (2048>>0)&0xff;

	return 0;
}

/*
 *@remark:   psm头的封装,里面的具体数据的填写已经占位，可以参考标准
 *@param :   pData  [in] 填充ps头数据的地址
 *@return:   0 success, others failed
*/
int gb28181_make_psm_header(pPSM_HEADER psmHeader)
{
	memset(psmHeader, 0, PSM_HDR_LEN);
	
	/* byte 0-2 */
	psmHeader->startCode1 = 0x00;		/* 0x00 */
	psmHeader->startCode2 = 0x00;		/* 0x00 */
	psmHeader->startCode3 = 0x01;		/* 0x01 */
	/* byte 3 */
	psmHeader->streamID = 0xbc;			/* stream ID */
	/* byte 4-5 */
	psmHeader->mapLen = htons(18-4);	/* program stream map length, not include the first 6 bytes */
	/* byte 6 */
	psmHeader->curNextIndicator = 1;	/* current next indicator */
	psmHeader->reserved1 = 3;			/* reserved */
	psmHeader->mapVersion = 0;			/* program stream map version */
	/* byte 7 */
	psmHeader->reserved2 = 0x7F;		/* reserved */
	psmHeader->mark = 1;				/* marker bit */
	/* byte 8-9 */
	psmHeader->infoLen = htons(0);		/* programe stream info length */
	/* byte 10-11 */
	psmHeader->eleMapLen = htons(4);	/* elementary stream map length */
	/* byte 12-15 */
	//psmHeader->aStreamType = 0x90;	/* audio stream_type */
	//psmHeader->aEleStreamID = 0xC0;	/* audio elementary_stream_id */
	//psmHeader->aEleInfoLen = htons(0);/* audio elementary_stream_info_length */
	/* byte 16-19 */
	psmHeader->vStreamType = 0x1B;		/* video stream_type */
	psmHeader->vEleStreamID = 0xE0;		/* video elementary_stream_id */
	psmHeader->vEleInfoLen = htons(0);	/* video elementary_stream_info_length */
	/* byte 20-23 */
	psmHeader->crc = mpeg_crc32((unsigned char*)psmHeader, PSM_HDR_LEN-4);	/* crc */
	
	return 0;
}

/*
 *@remark:   pes头的封装,里面的具体数据的填写已经占位，可以参考标准
 *@param :   pData      [in] 填充ps头数据的地址
 *           stream_id  [in] 码流类型
 *           paylaod_len[in] 负载长度
 *           pts        [in] 时间戳
 *           dts        [in]
 *@return:   0 success, others failed
 */
int gb28181_make_pes_header(pPES_HEADER pesHeader, unsigned char stream_id, int payload_len, unsigned long long pts, unsigned char aligFlag, unsigned char ptsFlag)
{
	memset(pesHeader, 0, PES_HDR_LEN);
	aligFlag &= 0x1;
	
	/* byte 0-2 */
	pesHeader->startCode1 = 0x00;			/* 0x00 */
	pesHeader->startCode2 = 0x00;			/* 0x00 */
	pesHeader->startCode3 = 0x01;			/* 0x01 */
	/* byte 3 */
	pesHeader->streamID = stream_id;		/* stream ID */
	/* byte 4-5 */
	if(ptsFlag)
		pesHeader->payloadLen = htons(payload_len+8);	/* pes分组中数据长度 + 该字节后的长度, pts置位, dts不置位 */
	else
		pesHeader->payloadLen = htons(payload_len+3);	/* pes分组中数据长度 + 该字节后的长度, pts不置位, dts不置位 */
	/* byte 6 */
	pesHeader->mark1 = 2;					/* always set to 10b */
	pesHeader->scramblingCtl = 0;			/* scrambling_control */
	pesHeader->priority = 1;				/* priority */
	pesHeader->aligtIndicator = aligFlag;	/* data_alignment_indicator */
	pesHeader->copyRight = 0;				/* set if copyrighted */
	pesHeader->original = 0;				/* original_or_copy */
	/* byte 7 该字节指出了有哪些可选字段 */
	if(ptsFlag)
		pesHeader->ptsFlag = 1;				/* PTS_flag */
	else
		pesHeader->ptsFlag = 0;				/* PTS_flag */
	pesHeader->dtsFlag = 0;					/* DTS_flag */
	pesHeader->escrFlag = 0;				/* ESCR_flag */
	pesHeader->esRateFlag = 0;				/* ES_rate_flag */
	pesHeader->dsmTrickMode = 0;			/* DSM_trick_mode_flag */
	pesHeader->addCpRtInfo = 0;				/* additional_copy_info_flag */
	pesHeader->crcFlag = 0;					/* PES_CRC_flag */
	pesHeader->extensionFlag = 0;			/* PES_extension_flag */
	/* byte 8 包含在 PES 分组标题中的可选字段和填充字节所占用的总字节数，从byte9开始计算 */
	if(ptsFlag)
		pesHeader->headLen = 5;				/* header_data_length */
	else
		pesHeader->headLen = 0;				/* header_data_length */
	/* byte 9-13 */
	pesHeader->ptsHeader = 3;				/* PTS(optional), 0010 if pts only, 0011 if dts fllows */
	pesHeader->pts1 = ((pts)>>30)&0x07;		/* [32-30] */
	pesHeader->mark2 = 1;					/* marker bit */
	pesHeader->pts2 = ((pts)>>23)&0xff;		/* [29-23] */
	pesHeader->pts3 = ((pts)>>15)&0xff;		/* [22-15] */
	pesHeader->mark3 = 1;					/* marker bit */
	pesHeader->pts4 = ((pts)>>7)&0xff;		/* [14-7] */
	pesHeader->pts5 = (pts)&0x7f;			/* [6-0] */
	pesHeader->mark4 = 1;					/* marker bit */
	/* byte 14-18 */
	//pesHeader->dtsHeader = 1;				/* DTS(optional), 0010 if pts only, 0011 if dts fllows */
	//pesHeader->dts1 = ((dts)>>30)&0x07;	/* [32-30] */
	//pesHeader->mark5 = 1;					/* marker bit */
	//pesHeader->dts2 = ((dts)>>23)&0xff;	/* [29-23] */
	//pesHeader->dts3 = ((dts)>>15)&0xff;	/* [22-15] */
	//pesHeader->mark6 = 1;					/* marker bit */
	//pesHeader->dts4 = ((dts)>>7)&0xff;	/* [14-7] */
	//pesHeader->dts5 = (dts)&0x7f;			/* [6-0] */
	//pesHeader->mark7 = 1;					/* marker bit */
	
	return 0;
}

/*
 *@remark:  音视频数据的打包成ps流，并封装成rtp
 *@param :  pData      [in] 需要发送的音视频数据
 *          nFrameLen  [in] 发送数据的长度
 *          pPacker    [in] 数据包的一些信息，包括时间戳，rtp数据buff，发送的socket相关信息
 *          stream_type[in] 数据类型 0 视频 1 音频
 *@return:  0 success others failed
 *@备注：	H264一般的规律为SPS-PPS-I-P...P--SPS-PPS-I-P...P----------SPS-PPS-I-P...P----------SPS-PPS-I-P...P
 *			序列参数集SPS（NaluType=7）前添加PS头和PSM头，PPS（NaluType=8）直接添加PES头即可
 *			封装为PS后，规律为PS-PSM-PES(SPS)-PES(PPS)-PES(I)-PES(P)……PES(P)---------PS-PSM-PES(SPS)-PES(PPS)-PES(I)-PES(P)……PES(P)
**/
int gb28181_streampackageForH264(pData_Info_s pPacker)
{
	int nSize = 0;
	u8 aligFlag = 1;
	int nFrameLen = pPacker->nFrameLen;
	u8 nFrameType = pPacker->nFrameType;
	u8 ptsFlag = pPacker->ptsFlag;
	char *pBuff = pPacker->szBuff;

	if( 7==pPacker->NaluType )
	{
		gb28181_make_ps_header(&(pPacker->psHeader), pPacker->s64CurPts);
		write(pPacker->sockfd, &(pPacker->psHeader), PS_HDR_LEN);
		gb28181_make_psm_header(&(pPacker->psmHeader));
		write(pPacker->sockfd, &(pPacker->psmHeader), PSM_HDR_LEN);
	}
	else if( 8==pPacker->NaluType || 6==pPacker->NaluType )
	{
		
	}
	else if(5==pPacker->NaluType)	//I帧非I帧都需要添加PS头
	{
		*(pBuff+4) &= (~0x60);
		*(pBuff+4) |= 0x60;
		gb28181_make_ps_header(&(pPacker->psHeader), pPacker->s64CurPts);
		write(pPacker->sockfd, &(pPacker->psHeader), PS_HDR_LEN);
		gb28181_make_psm_header(&(pPacker->psmHeader));
		write(pPacker->sockfd, &(pPacker->psmHeader), PSM_HDR_LEN);
	}
	else if(1==pPacker->NaluType)
	{
		*(pBuff+4) &= (~0x60);
		*(pBuff+4) |= 0x40;
		gb28181_make_ps_header(&(pPacker->psHeader), pPacker->s64CurPts);
		write(pPacker->sockfd, &(pPacker->psHeader), PS_HDR_LEN);
	}
	else	//其他NALU类型直接返回，暂不打入PS流中
	{
		printf("NaluType=%d, return!!!!!!!!!!!!\n", pPacker->NaluType);
		return 0;
	}
	
	while(nFrameLen > 0)
	{
		//每次帧的长度不要超过short类型，过了就得分片进循环行发送
		nSize = (nFrameLen > PS_PES_PAYLOAD_SIZE) ? PS_PES_PAYLOAD_SIZE : nFrameLen;
		gb28181_make_pes_header(&(pPacker->pesHeader), nFrameType ? 0xC0:0xE0, nSize, pPacker->s64CurPts, aligFlag, ptsFlag);
		write(pPacker->sockfd, &(pPacker->pesHeader), ptsFlag?PES_HDR_LEN:(PES_HDR_LEN-5));
		write(pPacker->sockfd, pBuff, nSize);
		nFrameLen -= nSize;
		pBuff += nSize;
		if(aligFlag)		//第一个PES包置位aligFlag，后面的分包置为0
		{
			aligFlag = 0;
		}
	}
	return 0;
}




