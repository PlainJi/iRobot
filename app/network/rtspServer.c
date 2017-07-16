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
#include "md5.h"

static char localipStr[20] = {0};
static char dateHeaderStr[64] = {0};
RTSP_CLIENT g_rtspClients[MAX_RTSP_CLIENT];
extern DEV_CFG_PARAM devCfgParam;

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

int ParseRequestString(char *reqStr, u32 len,
						char *cmdName,
						char *urlPreSuffix,
						char *urlSuffix,
		                char *cseq,
		                char *username,
		                char *response)
{
	int tempLen = 0;
	char *p = reqStr;
	char *q = NULL;
	char *r = NULL;
	int maxLen = PARAM_STRING_MAX -1;

	//get cmd lind
	tempLen = 0;
	q = cmdName;
	while(*p!='\0' && *p!=' ' && *p!='\t' && tempLen++<maxLen)
		*q++ = *p++;
	while(*p == ' ' || *p == '\t')
		p++;
	//get url
	r = p;
	if (r==strstr(p, "rtsp:/")) {
		tempLen = 0;
		q = urlPreSuffix;
		while(*p!='\0' && *p != ' ' && *p != '\t' && tempLen++<maxLen)
			*q++ = *p++;
	}
	//get cseq
	if (NULL!=(r=strstr(p, "CSeq:"))) {
		p = r;
		while(*p!='\0' && *p != ' ' && *p != '\t')
			p++;
		while(*p == ' ' || *p == '\t')
			p++;
		tempLen = 0;
		q = cseq;
		while(*p!='\0' && *p!=' ' && *p!='\t' && *p!='\r' && *p!='\n' && tempLen++<maxLen)
			*q++ = *p++;
	}
	//get auth line
	if (NULL!=(r=strstr(p, "Authorization: Digest"))) {
		p = r;
		if (NULL!=(r=strstr(p, "username="))) {
			tempLen = 0;
			p = r + 10;
			q = username;
			while(*p != '"' && tempLen++<maxLen)
				*q++ = *p++;
		}
		//printf("usr=%s\n", username);
		if (NULL!=(r=strstr(p, "response="))) {
			tempLen = 0;
			p = r + 10;
			q = response;
			while(*p != '"' && tempLen++<maxLen)
				*q++ = *p++;
		}
		//printf("rsp=%s\n", response);
	}

	//if (strlen(cmdName) && strlen(urlPreSuffix) && strlen(cseq)) {return 0;}
	if (strlen(cmdName) && strlen(cseq)) {return 0;}
	else {return -3;}
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

//md5(md5(username:realm:password):nonce:md5(public_method:url));
int describeCheck(RTSP_CLIENT *pClient, char *url, char *username, char *response)
{
	unsigned char temp[128] = {0};
	unsigned char md5_1[33] = {0};
	unsigned char md5_2[33] = {0};
	unsigned char res[33] = {0};

	if (32!=strlen(response)) {
		return -1;
	}
	if (!strcmp((const char*)devCfgParam.rtspParam.username, (const char*)username)) {
		sprintf((char*)temp,"%s:%s:%s", username, pClient->realm, devCfgParam.rtspParam.password);
		memset(md5_1, 0, sizeof(md5_1));
		md5_enc(temp, md5_1);
		
		sprintf((char*)temp,"DESCRIBE:%s", url);
		memset(md5_2, 0, sizeof(md5_2));
		md5_enc(temp, md5_2);

		sprintf((char*)temp,"%s:%s:%s", md5_1, pClient->nonce, md5_2);
		memset(res, 0, sizeof(res));
		md5_enc(temp, res);

		if (strcmp((const char*)response, (const char*)res))
			return -3;
	} else
		return -2;

	return 0;
}

int describeAnswerAuth(char *cseq, RTSP_CLIENT *pClient, char *urlSuffix)
{
	int ret = 0;
	const char *realm = "PlainServer1.0";
	u8 md5[16] = {0};
	char buf[1024] = {0};
	
	if (!pClient->socketRtsp){
		return -1;
	}

	memset(md5, 0, 16);
	memset(buf, 0, 1024);
	memset(pClient->realm, 0, sizeof(pClient->realm));
	strcpy(pClient->realm, realm);
	md5_enc((u8*)dateHeader(), md5);
	sprintf(pClient->nonce, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
		md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7], 
		md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);
	sprintf(buf,"RTSP/1.0 401 Unauthorized\r\n"
				"CSeq: %s\r\n"
				"%s"
				"WWW-Authenticate:Digest realm=\"%s\", nonce=\"%s\"\r\n\r\n", 
				cseq, dateHeader(), realm, pClient->nonce);
	if(socket_tcpSend(pClient->socketRtsp, buf, strlen(buf), NULL) < 0) {
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
	pTemp += sprintf(pTemp, "Content-Base: %s\r\n", urlSuffix);
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
	char cmdName[PARAM_STRING_MAX];
	char urlPreSuffix[PARAM_STRING_MAX];
	char urlSuffix[PARAM_STRING_MAX];
	char cseq[PARAM_STRING_MAX];
	char username[PARAM_STRING_MAX];
	char response[PARAM_STRING_MAX];
	
	int socketStream = 0;
	int s32Socket_opt_value = 1;
	struct sockaddr_in clientAddr;

	printf("######## RTSP:Create Client %s\n", pClient->clientIP);
	getLocalIP(pClient->socketRtsp, localipStr);
	
	while(pClient->status != RTSP_IDLE) {
		// recv cmd
		memset(pRecvBuf,0,sizeof(pRecvBuf));
		if( (len=socket_tcpRecv(pClient->socketRtsp, pRecvBuf, RTSP_RECV_SIZE, NULL)) <= 0) {
			printf("######## RTSP:Recv Error len=%d\n", len);
			break;
		}

		memset(cmdName,0,sizeof(cmdName));
		memset(urlPreSuffix,0,sizeof(urlPreSuffix));
		memset(urlSuffix,0,sizeof(urlSuffix));
		memset(cseq,0,sizeof(cseq));
		memset(username,0,sizeof(username));
		memset(response,0,sizeof(response));
		printf("######## RTSP:Receive message\n%s\n", pRecvBuf);
		if(ParseRequestString(pRecvBuf, len, cmdName, urlPreSuffix, urlSuffix, cseq, username, response)<0) {
			printf("######## RTSP:ParseRequestString error!\n");
			break;
		}
		
		printf("######## RTSP:answer cmd [%s]\n", cmdName);
		if(strstr(cmdName, "OPTIONS")) {
			optionAnswer(cseq, pClient->socketRtsp);
		}
		else if(strstr(cmdName, "DESCRIBE")) {
			if (!strlen(username) || !strlen(response))
				describeAnswerAuth(cseq, pClient, urlSuffix);
			else {
				if (describeCheck(pClient, urlPreSuffix, username, response))
					describeAnswerAuth(cseq, pClient, urlSuffix);
				else
					describeAnswer(cseq, pClient->socketRtsp, urlSuffix);
			}
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





