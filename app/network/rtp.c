#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include "include.h"

extern DEV_CFG_PARAM devCfgParam;

u64 getCurMS(void)
{
	struct timeb tb;
	ftime(&tb);
	return 1000ULL * tb.time + tb.millitm;
}

static int isStartCode3(u8 *buf)
{
	if (buf[0] != 0 || buf[1] != 0 || buf[2] != 1)					// 0x000001
		return 0;
	else
		return 1;
}

static int isStartCode4(u8 *buf)
{
	if (buf[0] != 0 || buf[1] != 0 || buf[2] != 0 || buf[3] != 1)	// 0x00000001
		return 0;
	else
		return 1;
}

//nalu->buf exclude 00 00 00 01 or 00 00 01
int getNalu(u8 *buf, u32 len, NALU *nalu)
{
	if (isStartCode3(buf)) {
		nalu->prefixLen = 3;
	}else if (isStartCode4(buf)) {
		nalu->prefixLen = 4;
	}else {
		printf("!!! No any startcode found\n");
		return -1;
	}

	nalu->buf = (u8*)((int)buf + nalu->prefixLen);			// exclude the start code
	nalu->len = len - nalu->prefixLen;
	nalu->nalForbiddenBit = (nalu->buf[0] & 0x80) >> 7;		// 1 bit, 0b1000 0000
	nalu->nalReferenceIDC = (nalu->buf[0] & 0x60) >> 5;		// 2 bit, 0b0110 0000
	nalu->nalType = (nalu->buf[0] & 0x1f);					// 5 bit, 0b0001 1111

	return 1;
}

void makeRTPHeader(RTP_HEADER *rtp_hdr, int seq, int ssrc, int timeStamp)
{
	memset(rtp_hdr, 0, sizeof(RTP_HEADER));
	rtp_hdr->payload = H264;
    rtp_hdr->version = 2;
    rtp_hdr->marker = 1;
	rtp_hdr->seq_no = htons(seq);
	rtp_hdr->ssrc = htonl(ssrc);
	rtp_hdr->timeStamp = htonl(timeStamp);
}

int sendSPSPPS(RTSP_CLIENT *pClient)
{
	char buf[128] = {0};
	u8 *p = NULL;
	u32 len = 0;
	RTP_HEADER rtp_hdr;

	//SPS
	memset(buf, 0, sizeof(buf));
	makeRTPHeader(&rtp_hdr, 0, pClient->ssrc, 0);
	memcpy(buf, &rtp_hdr, sizeof(RTP_HEADER));
	get_SPS(&p, &len);
	memcpy(buf+sizeof(RTP_HEADER), p+4, len-4);
	if(socket_tcpSend(pClient->socketStream, buf, sizeof(RTP_HEADER)+len-4, NULL)<0) {
		return -1;
	}

	//PPS
	memset(buf, 0, sizeof(buf));
	makeRTPHeader(&rtp_hdr, 1, pClient->ssrc, 0);
	memcpy(buf, &rtp_hdr, sizeof(RTP_HEADER));
	get_PPS(&p, &len);
	memcpy(buf+sizeof(RTP_HEADER), p+4, len-4);
	if(socket_tcpSend(pClient->socketStream, buf, sizeof(RTP_HEADER)+len-4, NULL)<0) {
		return -1;
	}

	return 0;
}

void getFU_A(NALU *nalu, RTP_HEADER *rtpHeader, int startOrEnd, u8 *buf, int len, char *sendBuf, int *sendLen)
{
	FU_INDICATOR fuIndicator;
	FU_HEADER fuHeader;
	memset(sendBuf, 0, MAX_PACKET_LEN+14);
	
	fuIndicator.F = nalu->nalForbiddenBit;
	fuIndicator.NRI = nalu->nalReferenceIDC;
	fuIndicator.TYPE = 28;
	
	if(0 == startOrEnd) {
		fuHeader.S = 0;
		fuHeader.E = 0;
	}
	else if(1 == startOrEnd) {
		fuHeader.S = 1;
		fuHeader.E = 0;
	}else if(2 == startOrEnd) {
		fuHeader.S = 0;
		fuHeader.E = 1;
	}
	fuHeader.R = 0;
	fuHeader.TYPE = nalu->nalType;
	
	memcpy(sendBuf, rtpHeader, sizeof(RTP_HEADER));
	memcpy(sendBuf+sizeof(RTP_HEADER), &fuIndicator, sizeof(FU_INDICATOR));
	memcpy(sendBuf+sizeof(RTP_HEADER)+sizeof(FU_INDICATOR), &fuHeader, sizeof(FU_HEADER));
	memcpy(sendBuf+sizeof(RTP_HEADER)+sizeof(FU_INDICATOR)+sizeof(FU_HEADER), buf, len);
	*sendLen = sizeof(RTP_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER) + len;
}

void sendNalu(int fd, NALU *nalu, u32 ssrc, u32 timeStamp, u32 *seqNo)
{
	RTP_HEADER rtpHeader;
	NALU_HEADER naluHeader;		// for single packet
	FU_INDICATOR fuIndicator;	// for fragmented packet
	FU_HEADER fuHeader;			// for fragmented packet
	u8 *buf = nalu->buf + 1;				//去掉视频流中的NALU头
	int len = nalu->len - 1;
	int sendLen = 0;
	char sendBuf[MAX_PACKET_LEN+14] = {0};
	
	memset(&rtpHeader, 0, sizeof(RTP_HEADER));
	rtpHeader.version = 2;
	rtpHeader.payload = H264;
	rtpHeader.timeStamp = htonl(timeStamp);
	rtpHeader.ssrc = htonl(ssrc);
	
	// single packet	RTP_HEADER | NALU_HEADER
	if(len <= MAX_PACKET_LEN) {
		rtpHeader.seq_no = htons((*seqNo)++);
		rtpHeader.marker = 1;
		
		naluHeader.F = nalu->nalForbiddenBit;
		naluHeader.NRI = nalu->nalReferenceIDC;
		naluHeader.TYPE = nalu->nalType;
		
		memcpy(sendBuf, &rtpHeader, sizeof(RTP_HEADER));
		memcpy(sendBuf+sizeof(RTP_HEADER), &naluHeader, sizeof(NALU_HEADER));
		memcpy(sendBuf+sizeof(RTP_HEADER)+sizeof(NALU_HEADER), buf, len);
		sendLen = sizeof(RTP_HEADER) + sizeof(NALU_HEADER) + len;
		if(socket_tcpSend(fd, sendBuf, sendLen, NULL)<0) {
			return;
		}
	}else {
	// fragmented packet		RTP_HEADER | FU_INDICATOR | FU_HEADER
		// start
		rtpHeader.seq_no = htons((*seqNo)++);
		rtpHeader.marker = 0;
		getFU_A(nalu, &rtpHeader, 1, buf, MAX_PACKET_LEN, sendBuf, &sendLen);
		if(socket_tcpSend(fd, sendBuf, sendLen, NULL)<0) {
			return;
		}
		len -= MAX_PACKET_LEN;
		buf += MAX_PACKET_LEN;
		
		// middle
		while(len > MAX_PACKET_LEN) {
			rtpHeader.seq_no = htons((*seqNo)++);
			rtpHeader.marker = 0;
			getFU_A(nalu, &rtpHeader, 0, buf, MAX_PACKET_LEN, sendBuf, &sendLen);
			if(socket_tcpSend(fd, sendBuf, sendLen, NULL)<0) {
				return;
			}
			len -= MAX_PACKET_LEN;
			buf += MAX_PACKET_LEN;
		}
		
		// end
		rtpHeader.seq_no = htons((*seqNo)++);
		rtpHeader.marker = 1;
		getFU_A(nalu, &rtpHeader, 2, buf, len, sendBuf, &sendLen);
		if(socket_tcpSend(fd, sendBuf, sendLen, NULL)<0) {
			return;
		}
	}
}

void rtp_preview_task(void *param)
{
	RTSP_CLIENT *pClient = (RTSP_CLIENT*)param;
	int *status = &pClient->status;

	FRAME_INFO frameInfo;
	NALU nalu;
	int firstFlag = 1;
	u32 timeStamp = 0;
	u32 seqNo = 2;
	
	volatile u32 *wIdx = NULL;
	u32 rIdx = 0;
	ENCODE_BUF *encodeBuf = NULL;
	get_encode_buf(&encodeBuf);
	wIdx = &encodeBuf->writeIdx;
	pClient->ssrc = 0x11223344;
	
	encode_force_I_Frame();
	while(1) {
		rIdx = *wIdx-1;
		if(1 == check_I_frame(rIdx)) {
			break;
		}
		usleep(20000);
	}

	if(!get_spspps_state()) {
		goto exit;
	}
	sendSPSPPS(pClient);

	timeStamp = 0;
	while(1) {
		
		if(RTSP_SENDING != *status) {
			break;
		}
		
		if(!firstFlag && *wIdx == rIdx) {
			usleep(20000);
			continue;
		}else {
			firstFlag = 0;
		}

		get_one_frame(rIdx, &frameInfo);
		//printf("get [%3d] [%d] %6d %02X %02X %02X %02X %02X\n", rIdx, frameInfo.type, frameInfo.len, 
		//		frameInfo.p[0], frameInfo.p[1], frameInfo.p[2], frameInfo.p[3], frameInfo.p[4]);
		getNalu(frameInfo.p, frameInfo.len, &nalu);
		sendNalu(pClient->socketStream, &nalu, pClient->ssrc, timeStamp, &seqNo);
		IDX_INCREASE(rIdx);
		timeStamp += 90000/devCfgParam.encParam.fps;
	}

exit:
	printf("Exit rtp_preview_task.\n");
}


