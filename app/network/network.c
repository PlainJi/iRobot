#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include <errno.h>
#include <signal.h>
#include <fcntl.h> 
#include <pthread.h>
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <linux/sockios.h>

#include "include/pack.h"
#include "include/network.h"
#include "include/encode.h"
#include "include/rtspServer.h"

int exitok = 0;
char localipStr[20] = {0};
char dateHeaderStr[64] = {0};
RTSP_CLIENT g_rtspClients[MAX_RTSP_CLIENT];

void *SendRtpPacketsToClient(void *param)
{
	int cnt = 50;
	UNUSED(param);
	
	while(cnt--)
	{
		printf("rtp send %d\n", cnt);
		sleep(1);
	}

	return NULL;
}

int sendSPS_PPS(int fd)
{
	char buf[100] = {0};
	int len = 0;

	len = getNetSPS(buf);
	if(len)
	{
		if(send(fd, buf, len, 0)!=len)
		{
			printf("sd to [%d] failed, size: %d, err: %s\n", fd, len, strerror(errno));
		}
	}
	else
	{
		printf("len error!\n");
		return 0;
	}

	len = getNetPPS(buf);
	if(len)
	{
		if(send(fd, buf, len, 0)!=len)
		{
			printf("sd to [%d] failed, size: %d, err: %s\n", fd, len, strerror(errno));
		}
	}
	
	return 0;
}

char *dateHeader(void)
{
	time_t tt = time(NULL);
	strftime(dateHeaderStr, sizeof(dateHeaderStr), "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
	return dateHeaderStr;
}

void GetLocalIP(int sock, char *localIp)
{
	struct ifreq ifreq;
	struct sockaddr_in *sin;
	
	strcpy(ifreq.ifr_name,"eth0");
	if (!(ioctl(sock, SIOCGIFADDR, &ifreq)))
	{
		sin = (struct sockaddr_in *)&ifreq.ifr_addr;
		sin->sin_family = AF_INET;
		strcpy(localIp, inet_ntoa(sin->sin_addr));
	}
}

char* strDupSize(char const* str) 
{
	if (str == NULL)
		return NULL;
	size_t len = strlen(str) + 1;
	char *copy = malloc(len);

	return copy;
}

int ParseRequestString( char const* reqStr,             unsigned reqStrSize,                char* resultCmdName,
		                unsigned resultCmdNameMaxSize,  char* resultURLPreSuffix,           unsigned resultURLPreSuffixMaxSize,
		                char* resultURLSuffix,          unsigned resultURLSuffixMaxSize,    char* resultCSeq,     
                        unsigned resultCSeqMaxSize) 
{
	// Read everything up to the first space as the command name:
	char c = 0;
	int parseSucceeded = FALSE;
	unsigned int i,j,k,k1,k2,k3,n;
	for (i = 0; i < resultCmdNameMaxSize-1 && i < reqStrSize; ++i) 
	{
		c = reqStr[i];
		if (c == ' ' || c == '\t') 
		{
			parseSucceeded = TRUE;
			break;
		}
		resultCmdName[i] = c;
	}
	resultCmdName[i] = '\0';
	if (!parseSucceeded) 
		return FALSE;

	// Skip over the prefix of any "rtsp://" or "rtsp:/" URL that follows:
	j = i+1;
	while (j < reqStrSize && (reqStr[j] == ' ' || reqStr[j] == '\t')) ++j; // skip over any additional white space
	//rtsp://192.168.1.5:554
	for (j = i+1; j < reqStrSize-8; ++j) 
	{
		if ((reqStr[j]  == 'r' || reqStr[j]   == 'R')
			&& (reqStr[j+1] == 't' || reqStr[j+1] == 'T')
			&& (reqStr[j+2] == 's' || reqStr[j+2] == 'S')
			&& (reqStr[j+3] == 'p' || reqStr[j+3] == 'P')
			&& reqStr[j+4]  == ':' && reqStr[j+5] == '/') 
		{
			j += 6;
			//printf("reqStr:%s__\n",&reqStr[j]);
			if (reqStr[j] == '/') 
			{
				// This is a "rtsp://" URL; skip over the host:port part that follows:
				++j;
				while (j < reqStrSize && reqStr[j] != '/' && reqStr[j] != ' ') 
				++j;
			} 
			else 
			{
				// This is a "rtsp:/" URL; back up to the "/":
				--j;
			}
			i = j;
			break;
		}
	}

	// Look for the URL suffix (before the following "RTSP/"):
	parseSucceeded = FALSE;
	for (k = i+1; k < reqStrSize-5; ++k) 
	{
		if (reqStr[k]   == 'R' && \
			reqStr[k+1] == 'T' && \
			reqStr[k+2] == 'S' && \
			reqStr[k+3] == 'P' && \
			reqStr[k+4] == '/')
		{
			while (--k >= i && reqStr[k] == ' '); // go back over all spaces before "RTSP/"
			k1 = k;
			while (k1 > i && reqStr[k1] != '/' && reqStr[k1] != ' ') --k1;
			// the URL suffix comes from [k1+1,k]

			// Copy "resultURLSuffix":
			if (k - k1 + 1 > resultURLSuffixMaxSize) 
				return FALSE; // there's no room
			n = 0;
			k2 = k1+1;
			while (k2 <= k)
				resultURLSuffix[n++] = reqStr[k2++];
			resultURLSuffix[n] = '\0';
			// Also look for the URL 'pre-suffix' before this:
			k3 = --k1;
			while (k3 > i && reqStr[k3] != '/' && reqStr[k3] != ' ') 
				--k3;
			// the URL pre-suffix comes from [k3+1,k1]

			// Copy "resultURLPreSuffix":
			if (k1 - k3 + 1 > resultURLPreSuffixMaxSize) 
				return FALSE; // there's no room
			n = 0; 
			k2 = k3+1;
			while (k2 <= k1) 
				resultURLPreSuffix[n++] = reqStr[k2++];
			resultURLPreSuffix[n] = '\0';

			i = k + 7; // to go past " RTSP/"
			parseSucceeded = TRUE;
			break;
		}
	}
	if (!parseSucceeded) 
		return FALSE;

	// Look for "CSeq:", skip whitespace,
	// then read everything up to the next \r or \n as 'CSeq':
	parseSucceeded = FALSE;
	for (j = i; j < reqStrSize-5; ++j) 
	{
		if (reqStr[j] == 'C' && reqStr[j+1] == 'S' && reqStr[j+2] == 'e' && reqStr[j+3] == 'q' && reqStr[j+4] == ':') 
		{
			j += 5;
			while (j < reqStrSize && (reqStr[j] ==  ' ' || reqStr[j] == '\t')) ++j;
			for (n = 0; n < resultCSeqMaxSize-1 && j < reqStrSize; ++n,++j)
			{
				c = reqStr[j];
				if (c == '\r' || c == '\n') 
				{
					parseSucceeded = TRUE;
					break;
				}
			resultCSeq[n] = c;
			}
			resultCSeq[n] = '\0';
			break;
		}
	}
	if (!parseSucceeded) 
		return FALSE;
	
	return parseSucceeded;
}

int OptionAnswer(char *cseq, int sock)
{
	int ret = 0;
	char buf[1024] = {0};
	
	if (sock != 0)
	{
		memset(buf, 0, 1024);
		sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n", cseq, dateHeader(), "OPTIONS,DESCRIBE,SETUP,PLAY,PAUSE,TEARDOWN");
		if((ret = send(sock, buf, strlen(buf), 0))<=0)
		{
			return FALSE;
		}
		else
		{
			printf("%s\n",buf);
		}
		return TRUE;
	}
	return FALSE;
}

int DescribeAnswer(char *cseq, int sock, char * urlSuffix, char* recvbuf)
{
	UNUSED(recvbuf);
	int ret = 0;
	char sdpMsg[2048] = {0};
	char buf[1024] = {0};
	char *pTemp = NULL;
	char *pTemp2 = NULL;
	
	if (sock != 0)
	{
		memset(buf, 0, sizeof(buf));
		memset(sdpMsg,0,sizeof(sdpMsg));
		
		pTemp = buf;
		pTemp += sprintf(pTemp, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n", cseq);
		pTemp += sprintf(pTemp, "%s", dateHeader());
		pTemp += sprintf(pTemp, "Content-Type: application/sdp\r\n");
		
		pTemp2 = sdpMsg;
		pTemp2 += sprintf(pTemp2, "v=0\r\n");
		pTemp2 += sprintf(pTemp2, "o=- 1000 0 IN IP4 %s\r\n", localipStr);
		pTemp2 += sprintf(pTemp2, "c=IN IP4 0.0.0.0\r\n");
		pTemp2 += sprintf(pTemp2, "a=tool: RtpRtspFlyer\r\n");
		pTemp2 += sprintf(pTemp2, "a=range:npt=0- \r\n");
		pTemp2 += sprintf(pTemp2, "m=video 0 RTP/AVP 96\r\n");
		pTemp2 += sprintf(pTemp2, "a=rtpmap:96 H264/90000\r\n");
		pTemp += sprintf(pTemp, "Content-Base: rtsp://%s/%s/\r\n", localipStr, urlSuffix);
		pTemp += sprintf(pTemp, "Content-length: %d\r\n\r\n", strlen(sdpMsg));
		strcat(pTemp, sdpMsg);
		
		if((ret = send(sock, buf, strlen(buf), 0))<=0)
		{
			return FALSE;
		}
		else
		{
			printf("%s\n", buf);
		}
	}

	return TRUE;
}

void ParseTransportHeader(char const *buf,
						StreamingMode *streamingMode,
						char **streamingModeString,
						char **destinationAddressStr,
						u8 *destinationTTL,
						u16 *clientRTPPortNum, // if UDP
						u16 *clientRTCPPortNum, // if UDP
						unsigned char *rtpChannelId, // if TCP
						unsigned char *rtcpChannelId // if TCP
						)
{
	*streamingMode = RTP_UDP;
	*streamingModeString = NULL;
	*destinationAddressStr = NULL;
	*destinationTTL = 255;
	*clientRTPPortNum = 0;
	*clientRTCPPortNum = 1; 
	*rtpChannelId = *rtcpChannelId = 0xFF;
	
	u16 p1, p2;
	char *field;
	char const *fields = buf + 11;
	unsigned int ttl, rtpCid, rtcpCid;
	
	//First find "Transport:"
	while (1) 
	{
		if (*buf == '\0') 
			return;
		if (strncasecmp(buf, "Transport: ", 11) == 0) 
			break;
		++buf;
	}
	
	//Then run through each of the fields, looking for ones we handle
	field = strDupSize(fields);
	while (sscanf(fields, "%[^;]", field) == 1)
	{
		if (strcmp(field, "RTP/AVP/TCP") == 0)
		{
			*streamingMode = RTP_TCP;
		}
		else if (strcmp(field, "RAW/RAW/UDP") == 0 || strcmp(field, "MP2T/H2221/UDP") == 0)
		{
			*streamingMode = RAW_UDP;
		}
		else if (strncasecmp(field, "destination=", 12) == 0)
		{
			free(destinationAddressStr);
		}
		else if (sscanf(field, "ttl%u", &ttl) == 1)
		{
			*destinationTTL = (u8)ttl;
		}
		else if (sscanf(field, "client_port=%hu-%hu", &p1, &p2) == 2)
		{
			*clientRTPPortNum = p1;
			*clientRTCPPortNum = p2;
		}
		else if (sscanf(field, "client_port=%hu", &p1) == 1)
		{
			*clientRTPPortNum = p1;
			*clientRTCPPortNum = *streamingMode == RAW_UDP ? 0 : p1 + 1;
		}
		else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2)
		{
			*rtpChannelId = (unsigned char)rtpCid;
			*rtcpChannelId = (unsigned char)rtcpCid;
		}
		
		fields += strlen(field);
		while (*fields == ';')
			++fields;
		if (*fields == '\0' || *fields == '\r' || *fields == '\n')
			break;
	}
	free(field);
}

int SetupAnswer(char *cseq, int sock, int SessionId, char *urlSuffix, char *recvbuf, int *rtpport, int *rtcpport)
{
	UNUSED(urlSuffix);
	int ret = 0;
	char buf[1024];
	memset(buf,0,1024);
	StreamingMode streamingMode;
	char *streamingModeString; // set when RAW_UDP streaming is specified
	char *clientsDestinationAddressStr;
	u8 clientsDestinationTTL;
	u16 clientRTPPortNum, clientRTCPPortNum;
	unsigned char rtpChannelId, rtcpChannelId;
	
	if (sock != 0)
	{
		ParseTransportHeader(recvbuf, &streamingMode, &streamingModeString,
			&clientsDestinationAddressStr, &clientsDestinationTTL,
			&clientRTPPortNum, &clientRTCPPortNum,			//UDP
			&rtpChannelId, &rtcpChannelId);					//TCP

		*rtpport = clientRTPPortNum;
		*rtcpport = clientRTCPPortNum;
		
		sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sTransport: RTP/AVP/UDP;unicast;client_port=%d-%d;server_port=%d-%d\r\nSession: %d\r\n\r\n",
			cseq, dateHeader(),
			clientRTPPortNum, clientRTCPPortNum,
			RTSP_SERVER_START_PORT, RTSP_SERVER_START_PORT+1,
			SessionId);
		if((ret = send(sock, buf, strlen(buf), 0))<=0)
		{
			return FALSE;
		}
		else
		{
			printf("%s",buf);
		}
		return TRUE;
	}
	return FALSE;
}

int PlayAnswer(char *cseq, int sock,int SessionId,char* urlPre,char* recvbuf)
{
	UNUSED(urlPre);
	UNUSED(recvbuf);
	int ret = 0;
	char buf[1024];
	memset(buf,0,1024);

	if (sock != 0)
	{
		sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %s\r\nServer: RtpRtspFlyer\r\nContent-length: 0\r\nSession: %d\r\n\r\n", cseq, SessionId);
		if((ret = send(sock, buf, strlen(buf), 0))<=0)
		{
			return FALSE;
		}
		else
		{
			printf("%s",buf);
		}
		return TRUE;
	}
	return FALSE;
}

int PauseAnswer(char *cseq,int sock,char *recvbuf)
{
	UNUSED(recvbuf);
	int ret = 0;
	char buf[1024];
	memset(buf,0,1024);
	
	if (sock != 0)
	{
		sprintf(buf,"RTSP/1.0 200 OK\r\nCSeq: %s\r\n%s\r\n\r\n", cseq, dateHeader());
	
		if((ret = send(sock, buf, strlen(buf), 0))<=0)
		{
			return FALSE;
		}
		else
		{
			printf("%s",buf);
		}
		return TRUE;
	}
	return FALSE;
}

int TeardownAnswer(char *cseq,int sock,int SessionId,char *recvbuf)
{
	UNUSED(recvbuf);
	int ret = 0;
	char buf[1024];
	memset(buf,0,1024);

	if (sock != 0)
	{
		sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sSession: %d\r\n\r\n", cseq, dateHeader(), SessionId);
	
		if((ret = send(sock, buf, strlen(buf), 0))<=0)
		{
			return FALSE;
		}
		else
		{
			printf("%s", buf);
		}
		return TRUE;
	}
	return FALSE;
}

void *RtspClientMsg(void *pParam)
{
	pthread_detach(pthread_self());
	RTSP_CLIENT * pClient = (RTSP_CLIENT*)pParam;
	char strIp[255];
	int  srcIP[4];
	int  strPort;
	char strOther[255];
	int nRes,Ret;
	int rtpport,rtcpport;
	char pRecvBuf[RTSP_RECV_SIZE];
	char cmdName[PARAM_STRING_MAX];
	char urlPreSuffix[PARAM_STRING_MAX];
	char urlSuffix[PARAM_STRING_MAX];
	char cseq[PARAM_STRING_MAX];
	struct sockaddr_in servaddr;
	
	int udpSockFd = 0;
	int s32Socket_opt_value = 1;
	struct sockaddr_in clientaddr;

	memset(pRecvBuf,0,sizeof(pRecvBuf));
	printf("######## RTSP:Create Client %s\n", pClient->clientIP);
	GetLocalIP(pClient->socket, localipStr);
	
	while(pClient->status != RTSP_IDLE)
	{
		nRes = recv(pClient->socket, pRecvBuf, RTSP_RECV_SIZE, 0);
		if(nRes < 1)
		{
			printf("######## RTSP:Recv Error %d\n", nRes);
			g_rtspClients[pClient->index].status = RTSP_IDLE;
			g_rtspClients[pClient->index].seqnum = 0;
			g_rtspClients[pClient->index].tsvid = 0;
			g_rtspClients[pClient->index].tsaud = 0;
			close(pClient->socket);
			break;
		}

		sscanf(pRecvBuf, "%*[^:]://%[^:]:%d/%s", strIp, &strPort, strOther);
		sscanf(strIp, "%d.%d.%d.%d", &srcIP[0], &srcIP[1], &srcIP[2], &srcIP[3]);

		printf("######## RTSP:Receive message\n%s\n", pRecvBuf);
		Ret = ParseRequestString(pRecvBuf, nRes, cmdName, sizeof(cmdName), urlPreSuffix, sizeof(urlPreSuffix), urlSuffix, sizeof(urlSuffix), cseq, sizeof(cseq));
		if(Ret==FALSE)
		{
			printf("######## RTSP:ParseRequestString error[%s]\n", pRecvBuf);
			g_rtspClients[pClient->index].status = RTSP_IDLE;
			g_rtspClients[pClient->index].seqnum = 0;
			g_rtspClients[pClient->index].tsvid = 0;
			g_rtspClients[pClient->index].tsaud = 0;
			close(pClient->socket);
			break;
		}
		
		printf("######## RTSP:answer cmd [%s]\n", cmdName);
		if(strstr(cmdName, "OPTIONS"))
		{
			OptionAnswer(cseq, pClient->socket);
		}
		else if(strstr(cmdName, "DESCRIBE"))
		{
			DescribeAnswer(cseq, pClient->socket, urlSuffix, pRecvBuf);
		}
		else if(strstr(cmdName, "SETUP"))
		{
			SetupAnswer(cseq, pClient->socket, pClient->sessionid, urlPreSuffix, pRecvBuf, &rtpport, &rtcpport);
			g_rtspClients[pClient->index].rtpport[0] = rtpport;
			g_rtspClients[pClient->index].rtcpport = rtcpport;
			g_rtspClients[pClient->index].reqchn = atoi(urlPreSuffix);
			if(strlen(urlPreSuffix)<100)
				strcpy(g_rtspClients[pClient->index].urlPre, urlPreSuffix);
		}
		else if(strstr(cmdName, "PLAY"))
		{
			PlayAnswer(cseq, pClient->socket, pClient->sessionid, g_rtspClients[pClient->index].urlPre, pRecvBuf);
			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			servaddr.sin_port = htons(RTSP_SERVER_START_PORT+pClient->index*2);
			memset(&clientaddr, 0, sizeof(clientaddr));
			clientaddr.sin_family=AF_INET;
			clientaddr.sin_addr.s_addr=inet_addr(g_rtspClients[pClient->index].clientIP);
			clientaddr.sin_port=htons(g_rtspClients[pClient->index].rtpport[0]);
			if((udpSockFd = socket(AF_INET, SOCK_DGRAM, 0))<=0)
			{
				printf("creat udpSockFd failed!\n");
				return (void *)(-3);
			}
			if (setsockopt(udpSockFd, SOL_SOCKET, SO_REUSEADDR, &s32Socket_opt_value, sizeof(int)) == -1)
			{
				return (void *)(-4);
			}
			printf("######## RTSP: bind local port [%d]\n", ntohs(servaddr.sin_port));
			if((bind(udpSockFd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)))<0)
			{
				return (void *)(-5);
			}
			printf("######## RTSP: connectting to %s:%d\n", g_rtspClients[pClient->index].clientIP, g_rtspClients[pClient->index].rtpport[0]);
			if(connect(udpSockFd, (struct sockaddr *)&clientaddr, sizeof(clientaddr))<0)
			{
				printf("--- connect to client failed\n");
				close(udpSockFd);
				return NULL;
			}
			printf("######## RTSP: Network Opened\n");
			memcpy(&(g_rtspClients[pClient->index].clientaddr), &clientaddr, sizeof(clientaddr));
			g_rtspClients[pClient->index].streamSocket = udpSockFd;
			printf("######## RTSP: send SPS PPS\n");
			sendSPS_PPS(udpSockFd);
			g_rtspClients[pClient->index].sendIframe = 0;
			g_rtspClients[pClient->index].status = RTSP_SENDING;
			encode_force_Ipic();
		}
		else if(strstr(cmdName, "PAUSE"))
		{
			PauseAnswer(cseq, pClient->socket, pRecvBuf);
		}
		else if(strstr(cmdName, "TEARDOWN"))
		{
			TeardownAnswer(cseq, pClient->socket, pClient->sessionid, pRecvBuf);
			g_rtspClients[pClient->index].status = RTSP_IDLE;
			g_rtspClients[pClient->index].seqnum = 0;
			g_rtspClients[pClient->index].tsvid = 0;
			g_rtspClients[pClient->index].tsaud = 0;
			close(pClient->socket);
			close(g_rtspClients[pClient->index].streamSocket);
		}
		else
		{
			printf("######## RTSP:unsupport cmd [%s]\n", cmdName);
		}
		if(exitok)
		{
			return NULL;
		}
	}
	printf("######## RTSP:Client %s Exit\n", pClient->clientIP);
	return NULL;
}

int getClientSlot(RTSP_CLIENT ClientSlot[])
{
	int i;
	
	for(i=0; i<MAX_RTSP_CLIENT; i++)
	{
		if(ClientSlot[i].status == RTSP_IDLE)
		{
			break;
		}
	}
	if(i == MAX_RTSP_CLIENT)
	{
		i = -1;
	}
	
	return i;
}

void *RtspServerListen(void *pParam)
{
	UNUSED(pParam);
	int slot;
	int s32Socket;
	struct sockaddr_in servaddr;
	int s32CSocket;
	int s32Rtn;
	int s32Socket_opt_value = 1;
	struct sockaddr_in addrAccept;
	int nSessionId = 1000;
	int nAddrLen = sizeof(struct sockaddr_in);
	int nMaxBuf = 10 * 1024;
	struct sched_param sched;
	sched.sched_priority = 1;
	pthread_t threadIdlsn[MAX_RTSP_CLIENT] = {0};
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(RTSP_SERVER_PORT);
	
	if((s32Socket = socket(AF_INET, SOCK_STREAM, 0))<=0)
	{
		printf("creat socket failed!\n");
		return (void *)(-1);
	}
	if (setsockopt(s32Socket, SOL_SOCKET, SO_REUSEADDR, &s32Socket_opt_value, sizeof(int)) == -1)     
	{
		return (void *)(-2);
	}
	if((s32Rtn = bind(s32Socket, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)))<0)
	{
		return (void *)(-3);
	}
	if((s32Rtn = listen(s32Socket, 50))<0)
	{
		return (void *)(-4);
	}
	
	while (!exitok)
	{
		if((s32CSocket = accept(s32Socket, (struct sockaddr*)&addrAccept, (socklen_t *)&nAddrLen))<=0)
			continue;
		if((slot = getClientSlot(g_rtspClients))<0)
		{
			printf("######## RTSP: reach max clients!!!!!!\n");
			close(s32CSocket);
			continue;
		}
		
		printf("######## RTSP Client %s Connected...\n", inet_ntoa(addrAccept.sin_addr));
		memset(&g_rtspClients[slot],0,sizeof(RTSP_CLIENT));
		g_rtspClients[slot].index = slot;
		g_rtspClients[slot].socket = s32CSocket;
		g_rtspClients[slot].status = RTSP_CONNECTED;
		g_rtspClients[slot].sessionid = nSessionId++;
		strcpy(g_rtspClients[slot].clientIP, inet_ntoa(addrAccept.sin_addr));
		if(setsockopt(s32CSocket, SOL_SOCKET, SO_SNDBUF, (char*)&nMaxBuf, sizeof(nMaxBuf)) == -1)
			printf("######## RTSP: Enalarge socket sending buffer error !!!!!!\n");
		
		threadIdlsn[slot] = 0;
		pthread_create(&threadIdlsn[slot], NULL, RtspClientMsg, &g_rtspClients[slot]);
		pthread_setschedparam(threadIdlsn[slot], SCHED_RR, &sched);
	}

	printf("######## INIT_RTSP_Listen() Exit !! \n");
	return NULL;
}

void InitRtspServer(void)
{
	memset(g_rtspClients, 0, sizeof(RTSP_CLIENT)*MAX_RTSP_CLIENT);
}

void StartRtspServer(void)
{
	pthread_t threadId = 0;
	struct sched_param thdsched;
	thdsched.sched_priority = 2;
	
	pthread_create(&threadId, NULL, RtspServerListen, NULL);
	pthread_setschedparam(threadId, SCHED_RR, &thdsched);
}

void quit_func(int sig)
{
	UNUSED(sig);
	exitok = 1;
}

struct net_handle *net_open(struct net_param params)
{
	struct net_handle *handle = (struct net_handle *) malloc(sizeof(struct net_handle));
	if (!handle)
	{
		printf("--- malloc net handle failed\n");
		return NULL;
	}

	CLEAR(*handle);
	handle->params.type = params.type;
	handle->params.serip = params.serip;
	handle->params.serport = params.serport;

	printf("######## init rtsp Server\n");
	InitRtspServer();
	printf("######## start rtsp Server\n");
	StartRtspServer();

	return handle;
}

void net_close(struct net_handle *handle)
{
	free(handle);
	printf("+++ net_handle freed\n");
}

int getClientNum(void)
{
	int i = 0;
	int cnt = 0;
	
	for(i=0;i<MAX_RTSP_CLIENT;i++)
	{
		if(RTSP_SENDING == g_rtspClients[i].status)
		{
			cnt++;
		}
	}
	
	return cnt;
}

int net_send(struct net_handle *handle, void *data, int size, int type)
{
	int i = 0;
	UNUSED(handle);
	
	for(i=0;i<MAX_RTSP_CLIENT;i++)
	{
		if(RTSP_SENDING == g_rtspClients[i].status)
		{
			if(!g_rtspClients[i].sendIframe && 5!=type)
			{
				continue;
			}
			else
			{
				g_rtspClients[i].sendIframe = 1;
			}
			if(send(g_rtspClients[i].streamSocket, data, size, 0)!=size)
			{
				printf("sd to [%d] failed, size: %d, err: %s\n", i, size, strerror(errno));
			}
		}
	}

	return 0;
}

int net_recv(struct net_handle *handle, void *data, int size)
{
	return recv(handle->sktfd, data, size, 0);
}


