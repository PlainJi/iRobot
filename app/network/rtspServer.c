#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <netinet/if_ether.h>
#include <fcntl.h> 
#include <sys/ioctl.h>

#include "include.h"

static char localipStr[20] = {0};
static char dateHeaderStr[64] = {0};
RTSP_CLIENT g_rtspClients[MAX_RTSP_CLIENT];

char *dateHeader(void)
{
	time_t tt = time(NULL);
	strftime(dateHeaderStr, sizeof(dateHeaderStr), "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
	return dateHeaderStr;
}

void getLocalIP(int sock, char *localIp)
{
	struct ifreq ifreq;
	struct sockaddr_in *sin;
	
	strcpy(ifreq.ifr_name, "eth0");
	
	if (!(ioctl(sock, SIOCGIFADDR, &ifreq)))
	{
		sin = (struct sockaddr_in *)&ifreq.ifr_addr;
		sin->sin_family = AF_INET;
		strcpy(localIp, inet_ntoa(sin->sin_addr));
	}
}

char* strDupSize(const char *str) 
{
	if (str == NULL) {
		return NULL;
	}
	
	size_t len = strlen(str) + 1;
	char *copy = malloc(len);

	return copy;
}

int ParseRequestString( const char *reqStr, u32 len, char *cmdName, u32 cmdNameSize,
						char *urlPreSuffix, u32 urlPreSuffixSize, char *urlSuffix, u32 urlSuffixMaxSize,
		                char *sequence, u32 sequenceMaxSize)
{
	char c = 0;
	int parseSucceeded = FALSE;
	u32 i,j,k,k1,k2,k3,n;

	//parse cmd name
	for (i = 0; i < cmdNameSize-1 && i < len; ++i) {
		c = reqStr[i];
		if (c == ' ' || c == '\t') {
			parseSucceeded = TRUE;
			break;
		}
		cmdName[i] = c;
	}
	cmdName[i] = '\0';
	if (!parseSucceeded)
		return -1;

	//j = i+1;
	//while (j < len && (reqStr[j] == ' ' || reqStr[j] == '\t')) ++j;

	//rtsp://192.168.1.5:554
	for (j = i+1; j < len-8; ++j) {
		if ((reqStr[j]  == 'r' || reqStr[j]   == 'R')
			&& (reqStr[j+1] == 't' || reqStr[j+1] == 'T')
			&& (reqStr[j+2] == 's' || reqStr[j+2] == 'S')
			&& (reqStr[j+3] == 'p' || reqStr[j+3] == 'P')
			&& reqStr[j+4]  == ':' && reqStr[j+5] == '/') 
		{
			j += 6;
			if (reqStr[j] == '/') {		// This is a "rtsp://" URL
				++j;
				while (j < len && reqStr[j] != '/' && reqStr[j] != ' ') ++j;
			}else {						// This is a "rtsp:/" URL
				--j;
			}
			i = j;
			break;
		}
	}

	// Look for the URL suffix (before the following "RTSP/"):
	parseSucceeded = FALSE;
	for (k = i+1; k < len-5; ++k) 
	{
		if (reqStr[k]   == 'R' && \
			reqStr[k+1] == 'T' && \
			reqStr[k+2] == 'S' && \
			reqStr[k+3] == 'P' && \
			reqStr[k+4] == '/')
		{
			// go back over all spaces before "RTSP/"
			// the URL suffix comes from [k1+1,k]
			while (--k >= i && reqStr[k] == ' '); 
			k1 = k;
			while (k1 > i && reqStr[k1] != '/' && reqStr[k1] != ' ') --k1;

			// Copy urlSuffix
			if (k - k1 + 1 > urlSuffixMaxSize) return -1;
			n = 0;
			k2 = k1+1;
			while (k2 <= k) urlSuffix[n++] = reqStr[k2++];
			urlSuffix[n] = '\0';
			
			// Also look for the URL 'pre-suffix' before this:
			// the URL pre-suffix comes from [k3+1,k1]
			k3 = --k1;
			while (k3 > i && reqStr[k3] != '/' && reqStr[k3] != ' ') --k3;
			
			// Copy urlPreSuffix
			if (k1 - k3 + 1 > urlPreSuffixSize) return -1;
			n = 0; 
			k2 = k3+1;
			while (k2 <= k1) 
				urlPreSuffix[n++] = reqStr[k2++];
			urlPreSuffix[n] = '\0';

			// go past " RTSP/"
			i = k + 7;
			parseSucceeded = TRUE;
			break;
		}
	}
	if (!parseSucceeded) 
		return -1;

	// Look for "CSeq:", skip whitespace
	// then read everything up to the next \r or \n as 'CSeq':
	parseSucceeded = FALSE;
	for (j = i; j < len-5; ++j) {
		if (reqStr[j] == 'C' && reqStr[j+1] == 'S' && reqStr[j+2] == 'e' && reqStr[j+3] == 'q' && reqStr[j+4] == ':') {
			j += 5;
			while (j < len && (reqStr[j] ==  ' ' || reqStr[j] == '\t')) ++j;
			for (n = 0; n < sequenceMaxSize-1 && j < len; ++n,++j) {
				c = reqStr[j];
				if (c == '\r' || c == '\n') {
					parseSucceeded = TRUE;
					break;
				}
				sequence[n] = c;
			}
			sequence[n] = '\0';
			break;
		}
	}
	if (!parseSucceeded) 
		return -1;
	
	return 0;
}

int optionAnswer(char *cseq, int sock)
{
	int ret = 0;
	char buf[1024] = {0};
	
	if (!sock){
		return -1;
	}
	memset(buf, 0, 1024);
	sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n", cseq, dateHeader(), "OPTIONS,DESCRIBE,SETUP,PLAY,PAUSE,TEARDOWN");
	if(socket_tcpSend(sock, buf, strlen(buf), NULL) < 0) {
		return -1;
	}else {
		printf("%s\n", buf);
	}
	
	return 0;
}

int describeAnswer(char *cseq, int sock, char *urlSuffix)
{
	int ret = 0;
	char sdpMsg[2048] = {0};
	char buf[1024] = {0};
	char *pTemp = NULL;
	char *pTemp2 = NULL;
	
	if (!sock) {
		return -1;
	}

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

	if(socket_tcpSend(sock, buf, strlen(buf), NULL) < 0) {
		return -1;
	}else {
		printf("%s\n", buf);
	}
	
	return 0;
}

void parseTransportHeader(char const *buf, STREAM_MODE *streamMode, u8 *destTTL,
						u16 *portRtp, u16 *portRtcp,			// if UDP
						u8 *rtpChannelId, u8 *rtcpChannelId)	// if TCP
{
	*streamMode = RTP_UDP;
	*destTTL = 255;
	*portRtp = 0;
	*portRtcp = 1; 
	*rtpChannelId = *rtcpChannelId = 0xFF;
	
	u16 p1, p2;
	char *field;
	char const *fields = buf + 11;
	unsigned int ttl, rtpCid, rtcpCid;
	
	//First find "Transport:"
	while (1) {
		if (*buf == '\0') return;
		if (strncasecmp(buf, "Transport: ", 11) == 0) 
			break;
		++buf;
	}
	
	//Then run through each of the fields, looking for ones we handle
	field = strDupSize(fields);
	while (sscanf(fields, "%[^;]", field) == 1) {
		if (strcmp(field, "RTP/AVP/TCP") == 0) {
			*streamMode = RTP_TCP;
		}
		else if (strcmp(field, "RAW/RAW/UDP") == 0 || strcmp(field, "MP2T/H2221/UDP") == 0) {
			*streamMode = RAW_UDP;
		}
		else if (strncasecmp(field, "destination=", 12) == 0) {
			//free(destinationAddressStr);		//??
		}
		else if (sscanf(field, "ttl%u", &ttl) == 1) {
			*destTTL = (u8)ttl;
		}
		else if (sscanf(field, "client_port=%hu-%hu", &p1, &p2) == 2)
		{
			*portRtp = p1;
			*portRtcp = p2;
		}
		else if (sscanf(field, "client_port=%hu", &p1) == 1) {
			*portRtp = p1;
			*portRtcp = (*streamMode==RAW_UDP) ? 0 : (p1+1);
		}
		else if (sscanf(field, "interleaved=%u-%u", &rtpCid, &rtcpCid) == 2) {
			*rtpChannelId = (unsigned char)rtpCid;
			*rtcpChannelId = (unsigned char)rtcpCid;
		}
		
		fields += strlen(field);
		while (*fields == ';') ++fields;
		if (*fields == '\0' || *fields == '\r' || *fields == '\n')
			break;
	}
	free(field);
}

int setupAnswer(char *cseq, int sock, int sessionID, char *recvbuf, u16 *portRtp, u16 *portRtcp)
{
	int ret = 0;
	char buf[1024];
	memset(buf,0,1024);
	STREAM_MODE streamMode;
	u8 destTTL;
	u8 rtpChannelId, rtcpChannelId;
	
	if (!sock) {
		return -1;
	}

	parseTransportHeader(recvbuf, &streamMode, &destTTL,
		portRtp, portRtcp,
		&rtpChannelId, &rtcpChannelId);
	
	sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sTransport: RTP/AVP/UDP;unicast;client_port=%d-%d;server_port=%d-%d\r\nSession: %d\r\n\r\n",
		cseq, dateHeader(),
		*portRtp, *portRtcp,
		RTSP_SERVER_START_PORT, RTSP_SERVER_START_PORT+1,		//?? 可以总是这两个值吗
		sessionID);
		
	if(socket_tcpSend(sock, buf, strlen(buf), NULL) < 0) {
		return -1;
	}else {
		printf("%s\n", buf);
	}
	
	return 0;
}

int playAnswer(char *cseq, int sock, int sessionID)
{
	int ret = 0;
	char buf[1024];
	memset(buf,0,1024);

	if (!sock) {
		return -1;
	}

	sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %s\r\nServer: RtpRtspFlyer\r\nContent-length: 0\r\nSession: %d\r\n\r\n", cseq, sessionID);

	if(socket_tcpSend(sock, buf, strlen(buf), NULL) < 0) {
		return -1;
	}else {
		printf("%s\n", buf);
	}
	
	return 0;
}

int pauseAnswer(char *cseq, int sock)
{
	int ret = 0;
	char buf[1024];
	memset(buf,0,1024);
	
	if (!sock) {
		return -1;
	}
	sprintf(buf,"RTSP/1.0 200 OK\r\nCSeq: %s\r\n%s\r\n\r\n", cseq, dateHeader());

	if(socket_tcpSend(sock, buf, strlen(buf), NULL) < 0) {
		return -1;
	}else {
		printf("%s\n", buf);
	}
	
	return 0;
}

int teardownAnswer(char *cseq,int sock,int sessionID)
{
	int ret = 0;
	char buf[1024];
	memset(buf,0,1024);

	if (!sock) {
		return -1;
	}
	
	sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n%sSession: %d\r\n\r\n", cseq, dateHeader(), sessionID);

	if(socket_tcpSend(sock, buf, strlen(buf), NULL) < 0) {
		return -1;
	}else {
		printf("%s\n", buf);
	}
	
	return 0;
}

void rtspClientMsg(void *pParam)
{
	RTSP_CLIENT *pClient = (RTSP_CLIENT*)pParam;
	char strIp[255];
	int  strPort;
	char strOther[255];
	int len = 0;
	u16 portRtp,portRtcp;
	char pRecvBuf[RTSP_RECV_SIZE];
	char cmdName[16];
	char urlPreSuffix[PARAM_STRING_MAX];
	char urlSuffix[PARAM_STRING_MAX];
	char cseq[PARAM_STRING_MAX];
	
	int socketStream = 0;
	int s32Socket_opt_value = 1;
	struct sockaddr_in clientAddr;

	memset(pRecvBuf,0,sizeof(pRecvBuf));
	printf("######## RTSP:Create Client %s\n", pClient->clientIP);
	getLocalIP(pClient->socketRtsp, localipStr);
	
	while(pClient->status != RTSP_IDLE) {
		// recv cmd
		if( (len=socket_tcpRecv(pClient->socketRtsp, pRecvBuf, RTSP_RECV_SIZE, NULL)) <= 0) {
			printf("######## RTSP:Recv Error len=%d\n", len);
			break;
		}

		//get ip & port
		sscanf(pRecvBuf, "%*[^:]://%[^:]:%d/%s", strIp, &strPort, strOther);

		printf("######## RTSP:Receive message\n%s\n", pRecvBuf);
		if(ParseRequestString(pRecvBuf, len, cmdName, sizeof(cmdName), 
			urlPreSuffix, sizeof(urlPreSuffix), urlSuffix, sizeof(urlSuffix), 
			cseq, sizeof(cseq))<0) {
			printf("######## RTSP:ParseRequestString error[%s]\n", pRecvBuf);
			break;
		}
		
		printf("######## RTSP:answer cmd [%s]\n", cmdName);
		if(strstr(cmdName, "OPTIONS")) {
			optionAnswer(cseq, pClient->socketRtsp);
		}
		else if(strstr(cmdName, "DESCRIBE")) {
			describeAnswer(cseq, pClient->socketRtsp, urlSuffix);
		}
		else if(strstr(cmdName, "SETUP")) {
			setupAnswer(cseq, pClient->socketRtsp, pClient->sessionID, pRecvBuf, &portRtp, &portRtcp);
			pClient->portRtp = portRtp;
			pClient->portRtcp = portRtcp;
			pClient->requestChannel = atoi(urlPreSuffix);		//??
			if(strlen(urlPreSuffix)<100)						//??
				strcpy(pClient->urlPre, urlPreSuffix);			//??
		}
		else if(strstr(cmdName, "PLAY")) {
			playAnswer(cseq, pClient->socketRtsp, pClient->sessionID);
			
			memset(&clientAddr, 0, sizeof(clientAddr));
			clientAddr.sin_family=AF_INET;
			clientAddr.sin_addr.s_addr=inet_addr(pClient->clientIP);
			clientAddr.sin_port=htons(pClient->portRtp);

			if( (socketStream=socket_udp(NULL, RTSP_SERVER_START_PORT+pClient->index*2)) < 0) {
				break;
			}
			printf("######## RTSP: connectting to %s:%d\n", 
				pClient->clientIP, 
				pClient->portRtp);
			if(connect(socketStream, (struct sockaddr *)&clientAddr, sizeof(clientAddr))<0) {		//?? 是否可以去掉
				printf("--- connect to client failed\n");
				close(socketStream);
				break;
			}
			
			printf("######## RTSP: Network Opened\n");
			pClient->socketStream = socketStream;
			pClient->status = RTSP_SENDING;
			task_creat(NULL, 90, 8192, (FUNC)rtp_preview_task, pClient);
		}
		else if(strstr(cmdName, "PAUSE")) {
			pauseAnswer(cseq, pClient->socketRtsp);
		}
		else if(strstr(cmdName, "TEARDOWN")) {
			teardownAnswer(cseq, pClient->socketRtsp, pClient->sessionID);
			close(pClient->socketStream);
		}
		else {
			printf("######## RTSP:unsupport cmd [%s]\n", cmdName);
		}
	}

	close(pClient->socketRtsp);
	pClient->status = RTSP_IDLE;
	printf("######## RTSP:Client %s Exit\n", pClient->clientIP);
}

int getClientSlot(RTSP_CLIENT ClientSlot[])
{
	int i;
	
	for(i=0; i<MAX_RTSP_CLIENT; i++) {
		if(ClientSlot[i].status == RTSP_IDLE) {
			break;
		}
	}
	if(i == MAX_RTSP_CLIENT) {
		i = -1;
	}
	
	return i;
}

void rtsp_server_task(void)
{
	int slot;
	int socketServer = 0;
	int socketClient = 0;
	struct sockaddr_in clientAddr;
	int sessionID = 1000;
	
	printf("######## init rtsp Server\n");
	memset(g_rtspClients, 0, sizeof(RTSP_CLIENT)*MAX_RTSP_CLIENT);
	
	if((socketServer=socket_tcp(NULL, RTSP_SERVER_PORT))<0) {
		return;
	}
	if(socket_tcpListen(socketServer, 5) < 0) {
		return;
	}
	
	while (1) {
		if( (socketClient=socket_tcpAccept(socketServer, &clientAddr)) < 0)
			continue;
		if( (slot=getClientSlot(g_rtspClients)) < 0) {
			printf("######## RTSP: reach max clients!!!!!!\n");
			close(socketClient);
			continue;
		}
		
		printf("######## RTSP Client %s Connected...\n", inet_ntoa(clientAddr.sin_addr));
		memset(&g_rtspClients[slot],0,sizeof(RTSP_CLIENT));
		g_rtspClients[slot].index = slot;
		g_rtspClients[slot].socketRtsp = socketClient;
		g_rtspClients[slot].status = RTSP_CONNECTED;
		g_rtspClients[slot].sessionID = sessionID++;
		strcpy(g_rtspClients[slot].clientIP, inet_ntoa(clientAddr.sin_addr));

		task_creat(NULL, 60, 2048, (FUNC)rtspClientMsg, &g_rtspClients[slot]);
	}

	printf("######## INIT_RTSP_Listen() Exit !! \n");
}

/*
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
*/

/*
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
			if(send(g_rtspClients[i].socketStream, data, size, 0)!=size)
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
*/


