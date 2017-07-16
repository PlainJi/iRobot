#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#define MAX_RTSP_CLIENT			6
#define PARAM_STRING_MAX		33
#define MAX_LINE_LEN			256

#define RTP_H264				96
#define RTSP_SERVER_PORT		554
#define RTSP_RECV_SIZE			1024
#define RTSP_SERVER_START_PORT	21000

typedef enum stream_mode_m
{
	RTP_UDP,
	RTP_TCP,
	RAW_UDP
}STREAM_MODE;

typedef enum rtsp_status_m
{
	RTSP_IDLE = 0,
	RTSP_CONNECTED = 1,
	RTSP_SENDING = 2,
}RTSP_STATUS;

typedef struct rtsp_client_t
{
	int index;
	int ssrc;
	int sessionID;
	int authentication;
	char nonce[36];
	char realm[36];
	int socketRtsp;
	int socketStream;
	int portRtp;
	int portRtcp;
	char clientIP[16];
	char urlPre[MAX_LINE_LEN];
	int requestChannel;
	int status;			//intract status
}RTSP_CLIENT;

typedef struct{
	char username[32];
	char password[32];
}RTSP_PARAM;

void rtsp_server_task(void);

#endif
