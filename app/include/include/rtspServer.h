#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#define MAX_RTSP_CLIENT			6
#define PARAM_STRING_MAX		100

#define RTP_H264				96
#define RTSP_SERVER_PORT		554
#define RTSP_RECV_SIZE			1024
#define RTSP_SERVER_START_PORT	21000

typedef enum
{
	RTP_UDP,
	RTP_TCP,
	RAW_UDP
}StreamingMode;

typedef enum
{
	RTSP_IDLE = 0,
	RTSP_CONNECTED = 1,
	RTSP_SENDING = 2,
}RTSP_STATUS;

typedef struct
{
	int index;
	int socket;
	int streamSocket;
	struct sockaddr_in clientaddr;
	int reqchn;
	int seqnum;
	int seqnum2;
	unsigned int tsvid;
	unsigned int tsaud;
	int status;
	int sendIframe;
	int sessionid;
	int rtpport[2];
	int rtcpport;
	char clientIP[20];
	char urlPre[PARAM_STRING_MAX];
}RTSP_CLIENT;

#endif
