#ifndef H264_2_PS_H
#define H264_2_PS_H

#include "comdef.h"

#define	RTP_VERSION	2
#define RTP_HDR_LEN 12
#define PS_HDR_LEN  14
#define SYS_HDR_LEN 18
#define PSM_HDR_LEN 20
#define PES_HDR_LEN (19-5)		//pds不置位
#define PS_PES_PAYLOAD_SIZE	(5114)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef struct
{
	/* byte 0-3 */
	unsigned long startcode;	/* start codes */
	/* byte 4-8 */
	unsigned char scr2:2;		/* System Clock Reference [29..28] */
	unsigned char mark2:1;		/* marker bit */
	unsigned char scr1:3;		/* System Clock Reference [32..30] */
	unsigned char mark1:2;		/* marker bit '01b' */
	unsigned char scr3;			/* System Clock Reference [27..20] */
	unsigned char scr5:2;		/* System Clock Reference [14..13] */
	unsigned char mark3:1;		/* marker bit */
	unsigned char scr4:5;		/* System Clock Reference [19..15] */
	unsigned char scr6;			/* System Clock Reference [12..5] */
	unsigned char exScr1:2;		/* SCR extension [8..7] */
	unsigned char mark4:1;		/* marker bit */
	unsigned char scr7:5;		/* System Clock Reference [4..0] */
	/* byte 9 */
	unsigned char mark5:1;		/* marker bit */
	unsigned char exScr2:7;		/* SCR extension [6..0] */
	/* byte 10-12 */
	unsigned char mulRate1;		/* multiplex rate [21.14] */
	unsigned char mulRate2;		/* multiplex rate [13..6] */
	unsigned char mark6:2;		/* marker bit */
	unsigned char mulRate3:6;	/* multiplex rate [5..0] */
	/* byte 13 */
	unsigned char stuffing:3;	/* stuffing */
	unsigned char reserve:5;	/* reserved */
} PS_HEADER, *pPS_HEADER;		/* total 14 bytes */

/*system header*/
typedef struct
{
	/* byte 0-3 */
	unsigned long startCode;			/* start code */
	/* byte 4-5 */
	unsigned short headerLen;			/* header length(not include the first 6 bytes) */
	/* byte 6-8 */
	unsigned char rateBound1:7;			/* max value of the multiplex rates of all packs in the stream,total 22 bits */
	unsigned char mark1:1;				/* marker bit */
	unsigned char rateBound2;
	unsigned char mark2:1;				/* marker bit */
	unsigned char rateBound3:7;
	/* byte 9 */
	unsigned char cspsFlag:1;			/* set for constrained */
	unsigned char fixedFlag:1;			/* set for fixed bitrate */
	unsigned char audioBound:6;			/* max munber of audio streams in this program stream */
	/* byte 10 */
	unsigned char videoBound:5;			/* max munber of video streams in this ISO stream */
	unsigned char mark3:1;				/* marker bit */
	unsigned char videoLock:1;			/* system_video_lock_flag */
	unsigned char audioLock:1;			/* system_audio_lock_flag */
	/* byte 11 */
	unsigned char reserve:7;			/* set to FF */
	unsigned char packRtRstcFlag:1;		/* packet rate restriction flag */
	/* byte 12-14 */
	unsigned char aStreamID;			/* audio stream ID */
	unsigned char aBufSizeBound1:5;		/* audio P-STD buffer size bound */
	unsigned char aPstdBufBndScl:1;		/* audio P-STD buffer bound scale */
	unsigned char mark4:2;				/* marker bit */
	unsigned char aBufSizeBound2;
	/* byte 15-17 */
	unsigned char vStreamID;			/* video stream ID */
	unsigned char vBufSizeBound1:5;		/* video P-STD buffer size bound */
	unsigned char vPstdBufBndScl:1;		/* video P-STD buffer bound scale */
	unsigned char mark5:2;				/* marker bit */
	unsigned char vBufSizeBound2;
} SYS_HEADER, *pSYS_HEADER;				/* total 18 bytes */

//psm header
typedef struct
{
	/* byte 0-2 */
	unsigned char startCode1;			/* 0x00 */
	unsigned char startCode2;			/* 0x00 */
	unsigned char startCode3;			/* 0x01 */
	/* byte 3 */
	unsigned char streamID;				/* stream ID */
	/* byte 4-5 */
	unsigned short mapLen;				/* program stream map length, not include the first 6 bytes */
	/* byte 6 */
	unsigned char mapVersion:5;			/* program stream map version */
	unsigned char reserved1:2;			/* reserved */
	unsigned char curNextIndicator:1;	/* current next indicator */
	/* byte 7 */
	unsigned char mark:1;				/* marker bit */
	unsigned char reserved2:7;			/* reserved */
	/* byte 8-9 */
	unsigned short infoLen;				/* programe stream info length */
	/* byte 10-11 */
	unsigned short eleMapLen;			/* elementary stream map length */
	/* byte 12-15 */
	//unsigned char aStreamType;			/* audio stream_type */
	//unsigned char aEleStreamID;			/* audio elementary_stream_id */
	//unsigned short aEleInfoLen;			/* audio elementary_stream_info_length */
	/* byte 16-19 */
	unsigned char vStreamType;			/* video stream_type */
	unsigned char vEleStreamID;			/* video elementary_stream_id */
	unsigned short vEleInfoLen;			/* video elementary_stream_info_length */
	/* byte 20-23 */
	unsigned long crc;					/* crc */
} PSM_HEADER, *pPSM_HEADER;				/* total 24 bytes */
	
//pes 
typedef struct
{
	/* byte 0-2 */
	unsigned char startCode1;			/* 0x00 */
	unsigned char startCode2;			/* 0x00 */
	unsigned char startCode3;			/* 0x01 */
	/* byte 3 */
	unsigned char streamID;				/* stream ID */
	/* byte 4-5 */
	unsigned short payloadLen;			/* pes分组中数据长度 + 该字节后的长度 */
	/* byte 6 */
	unsigned char original:1;			/* original_or_copy */
	unsigned char copyRight:1;			/* set if copyrighted */
	unsigned char aligtIndicator:1;		/* data_alignment_indicator */
	unsigned char priority:1;			/* priority */
	unsigned char scramblingCtl:2;		/* scrambling_control */
	unsigned char mark1:2;				/* always set to 10b */
	/* byte 7 */
	unsigned char extensionFlag:1;		/* PES_extension_flag */
	unsigned char crcFlag:1;			/* PES_CRC_flag */
	unsigned char addCpRtInfo:1;		/* additional_copy_info_flag */
	unsigned char dsmTrickMode:1;		/* DSM_trick_mode_flag */
	unsigned char esRateFlag:1;			/* ES_rate_flag */
	unsigned char escrFlag:1;			/* ESCR_flag */
	unsigned char dtsFlag:1;			/* DTS_flag */
	unsigned char ptsFlag:1;			/* PTS_flag */
	/* byte 8 */
	unsigned char headLen;				/* header_data_length */
	/* byte 9-13 */
	unsigned char mark2:1;				/* marker bit */
	unsigned char pts1:3;				/* [32-30] */
	unsigned char ptsHeader:4;			/* PTS(optional), 0010 if pts only, 0011 if dts fllows */
	unsigned char pts2;					/* [29-22] */
	unsigned char mark3:1;				/* marker bit */
	unsigned char pts3:7;				/* [21-15] */
	unsigned char pts4;					/* [14-7] */
	unsigned char mark4:1;				/* marker bit */
	unsigned char pts5:7;				/* [6-0] */
	/* byte 14-18 */
	unsigned char mark5:1;				/* marker bit */
	unsigned char dts1:3;				/* [32-30] */
	unsigned char dtsHeader:4;			/* DTS(optional), 0010 if pts only, 0011 if dts fllows */
	unsigned char dts2;					/* [29-22] */
	unsigned char mark6:1;				/* marker bit */
	unsigned char dts3:7;				/* [21-15] */
	unsigned char dts4;					/* [14-7] */
	unsigned char mark7:1;				/* marker bit */
	unsigned char dts5:7;				/* [6-0] */
} PES_HEADER, *pPES_HEADER;				/* total 19 bytes */

typedef struct _Data_Info_s
{
	int sockfd;
	char *szBuff;
	unsigned int nFrameLen;
	unsigned char nFrameType;	//0.video 1.audio
	unsigned char ptsFlag;
	unsigned short NaluType;
	unsigned short LastNaluType;
	unsigned short u16CSeq;
	signed long long s64CurPts;
	unsigned long u32Ssrc;
	//RTP_HEADER rtpHeader;		//12bytes
	PS_HEADER psHeader;		//14bytes
	SYS_HEADER sysHeader;		//18bytes
	PSM_HEADER psmHeader;		//20bytes
	PES_HEADER pesHeader;		//19bytes
	char newFileFlag;
	unsigned long long fileSize;
}Data_Info_s, *pData_Info_s;

int gb28181_streampackageForH264(pData_Info_s pPacker);
int write_MP4_Head(pData_Info_s frameInfo);

#endif



