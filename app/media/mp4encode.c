#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <arpa/inet.h>
#include "mp4v2/mp4v2.h"

MP4TrackId m_videoId = 0;
MP4FileHandle mp4File = NULL;

int openMP4(char *name)
{
	MP4FileHandle m_file = 0;
	
	m_file = MP4CreateEx(name, 0, 1, 1, NULL, 0, NULL, 0);
	if ( m_file == MP4_INVALID_FILE_HANDLE)
	{
		printf("open file fialed.\n");
		return -1;
	}
	
	MP4SetTimeScale(m_file, 90000);
	mp4File = m_file;
	
	return 0;
}

int writeMP4(const unsigned char *pData, int size, int type)
{
	MP4FileHandle hMp4File;
		
	if(mp4File == NULL)
	{
		return -1;
	}
	if(pData == NULL)
	{
		return -1;
	}

	hMp4File = mp4File;
	
	if(type == 0x07) // sps
	{
		// 添加h264 track
		m_videoId = MP4AddH264VideoTrack(hMp4File, 90000, 90000 / 25, 
			1280, 720, pData[5], pData[6], pData[7], 3);
		if (m_videoId == MP4_INVALID_TRACK_ID)
		{
			printf("add video track failed.\n");
			return -1;
		}
		MP4SetVideoProfileLevel(hMp4File, 1); // Simple Profile @ Level 3
		MP4AddH264SequenceParameterSet(hMp4File, m_videoId, pData+4, size-4);
	}
	else if(type == 0x08) // pps
	{
		MP4AddH264PictureParameterSet(hMp4File, m_videoId, pData+4, size-4);
	}
	else if(type == 0x06 || type == 0x05 || type == 0x01)
	{
		// MP4 Nalu前四个字节表示Nalu长度
		*((unsigned int*)pData) = htonl(size-4);
		if(!MP4WriteSample(hMp4File, m_videoId, pData, size, MP4_INVALID_DURATION, 0, 1))
		{
			return -1;
		}
	}
	else
	{
		printf("unknown type = %d\n", type);
	}

	return 0;
}

int closeMP4()
{
	if(mp4File)
		MP4Close(mp4File, 0);
	printf("Close MP4 File.\n");

	return 0;
}












