#ifndef INCLUDE_RTPPACK_H_
#define INCLUDE_RTPPACK_H_

#include "comdef.h"

#define H264    96
#define MAX_RTP_SIZE 1420

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
typedef struct
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
    unsigned long timestamp;
    // bytes 8-11
    unsigned long ssrc;			// sequence number
}rtp_header;

struct pac_param
{
	int max_pkt_len;    // maximum packet length, better be less than MTU(1500)
	int ssrc;			// identifies the synchronization source, set the value randomly, with the intent that no two synchronization sources within the same RTP session will have the same SSRC
};

typedef struct
{
    unsigned char TYPE :5;
    unsigned char NRI :2;
    unsigned char F :1;
} nalu_header;			// 1 Bytes

typedef struct
{
    //byte 0
    unsigned char TYPE :5;
    unsigned char NRI :2;
    unsigned char F :1;
} fu_indicator;			// 1 BYTE

typedef struct
{
    //byte 0
    unsigned char TYPE :5;
    unsigned char R :1;
    unsigned char E :1;
    unsigned char S :1;
} fu_header;			// 1 BYTE

typedef struct
{
    int startcodeprefix_len;	// 0x000001 or 0x00000001
    char *data;					// nalu data
    int len;					// nalu length
    int forbidden_bit;			//
    int nal_reference_idc;		//
    int nal_unit_type;			// nalu types
} nalu_t;

struct pac_handle
{
    void *inbuf;
    char *next_nalu_ptr;
    int inbuf_size;
    int FU_counter;
    int last_FU_size;
    int FU_index;
    int inbuf_complete;
    int nalu_complete;
    nalu_t nalu;
    unsigned short seq_num;
    u32 ts_start_millisec;		// timestamp in millisecond
    u32 ts_current_sample;		// timestamp in 1/90000.0 unit

    struct pac_param params;
};

struct pac_handle;
u64 get_current_millisec(void);
int get_next_nalu(struct pac_handle *handle);
struct pac_handle *pack_open(struct pac_param params);
void pack_put(struct pac_handle *handle, void *inbuf, int isize);
int pack_get(struct pac_handle *handle, void *outbuf, int bufsize, int *outsize);
void pack_close(struct pac_handle *handle);

int getNetPPS(char *buf);
int getNetSPS(char *buf);

#endif
