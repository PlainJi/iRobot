#ifndef __RECORD_H
#define __RECORD_H

#define SIZE_PER_FILE	(256*1024*1024)

typedef struct record_info_t
{
	u32 fileNo;
	u32 fileSize;
}RECORD_INFO;

typedef struct
{
	char *recordPath;
	int sizePerFile;
}RECORD_PARAM;

void record_task(void);

#endif