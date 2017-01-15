#ifndef RTPPACK_H
#define RTPPACK_H

#include "comdef.h"

#define H264    96
#define MAX_PACKET_LEN	1400

/******************************************************************
RTP_FIXED_HEADER
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier            |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|            contributing source (CSRC) identifiers             |
|                             ....                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
******************************************************************/
typedef struct rtp_header_t
{
    // Little Endian
    // byte 0
    unsigned char csrc_len:4;	// contains the number of CSRC identifiers that follow the fixed header.
    unsigned char extension:1;	// extension bit
    unsigned char padding:1;	// padding
    unsigned char version:2;	// version, current is 2
    // byte 1
    unsigned char payload:7;	// payload
    unsigned char marker:1;		// marker
    // bytes 2, 3
    unsigned short seq_no;
    // bytes 4-7
    unsigned long timeStamp;
    // bytes 8-11
    unsigned long ssrc;			// sequence number
}RTP_HEADER;

typedef struct
{
    unsigned char TYPE :5;
    unsigned char NRI :2;
    unsigned char F :1;
}NALU_HEADER;			// 1 Bytes

typedef struct
{
    //byte 0
    unsigned char TYPE :5;
    unsigned char NRI :2;
    unsigned char F :1;
}FU_INDICATOR;			// 1 BYTE

typedef struct
{
    //byte 0
    unsigned char TYPE :5;
    unsigned char R :1;
    unsigned char E :1;
    unsigned char S :1;
}FU_HEADER;			// 1 BYTE

typedef struct
{
    u8 *buf;					// nalu data
    int len;					// nalu length
    int prefixLen;				// 0x000001 or 0x00000001
    int nalForbiddenBit;		//
    int nalReferenceIDC;		//
    int nalType;			// nalu types
}NALU;

typedef struct pac_handle_t
{
	void *inbuf;
	char *next_nalu_ptr;
	int inbuf_size;
	int FU_counter;
	int last_FU_size;
	int FU_index;
	int inbuf_complete;
	int nalu_complete;
	NALU nalu;
	u16 seq_num;
	u32 startMS;				// timestamp in millisecond
	u32 timeStamp;			// timestamp in 1/90000.0 unit
	u32 ssrc;
}PACK_HANDLE;

void rtp_preview_task(void *param);

#endif
