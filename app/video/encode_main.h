#ifndef __ENCODE_MAIN_H
#define __ENCODE_MAIN_H

#define BUFFER_SIZE			(8*1024*1024)
#define FRAME_INFO_CNT		(512)
#define IDX_INCREASE(idx)	(idx=(((idx+1)==FRAME_INFO_CNT)?0:(idx+1)))
#define IDX_DECREASE(idx)	(idx=((((int)idx-1)==(-1)) ? (FRAME_INFO_CNT-1) : (idx-1)))

typedef struct frame_info_t
{
	u8 *p;
	u32 len;
	u8 type;
	time_t time;
}FRAME_INFO;

typedef struct encode_buf_t
{
	volatile u32 writeIdx;
	u32 readIdxRecord;
	u8 *curWP;
	u32 written;
	u8 *h264Buf;
	FRAME_INFO frameInfo[FRAME_INFO_CNT];
	u32 totalFrames;
}ENCODE_BUF;

int encode_start(void);
void encode_stop(void);
void encode_task(void *fd);
void encode_exit(void);
int get_one_frame(u32 idx, FRAME_INFO *frameInfo);
void get_encode_buf(ENCODE_BUF **p);
int get_last_I_frame_Idx(u32 idx);
int check_I_frame(u32 idx);
u8 get_frame_type(u32 idx);

#endif
