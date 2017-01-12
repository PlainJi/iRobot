#include "include.h"

#define MAJOR_VERSION	2
#define SUB_VERSION		0
#define MINOR_VERSION	0

void prt_soft_version(void)
{
	char timeTab[32];
	time_t cur_time;
	struct tm *p_tm;
	cur_time = time(NULL);
	p_tm = localtime(&cur_time);
	sprintf((char*)timeTab, "%04d-%02d-%02d %02d:%02d:%02d", 
		p_tm->tm_year+1900, p_tm->tm_mon+1, p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
	
	printf("version: V%d.%d.%d build %s\n", MAJOR_VERSION, SUB_VERSION, MINOR_VERSION, timeTab);
}