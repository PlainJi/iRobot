#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "include.h"

int socket_udp(char *ip, short port)
{
	int sock, nOptval=1;
	struct sockaddr_in fromAddr;
	
	if((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0){
		perror("creat udp socket error");
		return -1;
	}

	if(port){
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&nOptval, sizeof(int)) < 0){
			perror("SO_REUSEADDR ERROR");
		}
		memset(&fromAddr,0,sizeof(fromAddr));
		fromAddr.sin_family = AF_INET;
		if(NULL == ip){
			fromAddr.sin_addr.s_addr = INADDR_ANY;
		}else{
			fromAddr.sin_addr.s_addr = inet_addr(ip);
		}
		fromAddr.sin_port = htons(port);
		
		if(bind(sock, (struct sockaddr*)&fromAddr, sizeof(fromAddr))<0){
			perror("bind error");
			close(sock);
			return -1;
		}
	}
	
	return sock;
}

int socket_udpRecv(int sockfd, char *buf, int len, struct sockaddr_in *fromAddr)
{
	int recvLen = 0;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	if((recvLen = recvfrom(sockfd, buf, len, 0, (struct sockaddr*)&fromAddr, &addrLen))<0){
		perror("recvfrom() error");
		close(sockfd);
		return -1;
	}

	return recvLen;
}

int socket_udpSend(int fd, char *buf, int len, struct sockaddr_in *toAddr)
{
	socklen_t addrLen = sizeof(struct sockaddr_in);
	if(sendto(fd, buf, len, 0, (struct sockaddr*)toAddr, addrLen) != len) {
		perror("sendto fail");
		return -1;
	}

	return len;
}

int writen(int fd, void *vptr, unsigned int len)
{
	int left = len;
	int written = 0;
 	char *p = (char *)vptr;

	while(left>0) {
		if((written = send(fd, p, left, MSG_NOSIGNAL)) < 0) {
			if(errno == EINTR){
				printf("EINTR");
				written = 0;
			}else {
				perror("Send error");
				return -1;
			}
		}
		left -= written;
		p += written;
	}

	return len;
}

int socket_tcp(char *ip, short port)
{
	int sockfd = 0;
	int addreuse = 1;
	struct sockaddr_in serverAddr, clientAddr;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("creat tcp sockfd failed");
		return -1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &addreuse, sizeof(int)) < 0) {
		perror("setsockopt");
		close(sockfd);
		return -1;
	}
	
	bzero((char *)&serverAddr, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	if(ip){
		serverAddr.sin_addr.s_addr = inet_addr(ip);
	}else{
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	serverAddr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind failed");
		return -1;
	}

	return sockfd;
}

int socket_tcpListen(int sockfd, int num)
{
	if (listen(sockfd, num) < 0) {
		perror("listen faile");
		return -1;
	}

	return 0;
}

int socket_tcpAccept(int sockfd, struct sockaddr_in *clientAddr)
{
	int fd = 0;
	socklen_t sin_size = sizeof(struct sockaddr_in);
	
    if ((fd = accept(sockfd, (struct sockaddr *)clientAddr, &sin_size)) == -1) { 
		perror("accept");
		return -1;
    }
	//printf("tcpServer: got connection from %s:%d\n", inet_ntoa(*clientAddr.sin_addr), ntohs(*clientAddr.sin_port));

    return fd;
}

int socket_tcpConnect(char *ip, short port)
{
	int sockfd = 0;
    struct sockaddr_in serverAddr;
    
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(ip);
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("creat tcp socket");
        return -1; 
    }
    
    if(connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        return -1;
    }

    return sockfd;
}

int socket_tcpSend(int fd, void *buf, u32 len, struct timeval *pTimeout)
{
	fd_set wset;

	if(pTimeout) {
		FD_ZERO(&wset);
		FD_SET(fd, &wset);
		if(select(fd+1, NULL, &wset, NULL, pTimeout) <= 0) {
			perror("select failed");
			return -1;
		}
		
		if(FD_ISSET(fd, &wset)){
			if (writen(fd, buf, len) != len) {
				return -1;
			}
		}
	}else {
		if (writen(fd, buf, len) != len) {
				return -1;
		}
	}

	return 0;
}

int socket_tcpRecv(int fd, char *buf, int len, struct timeval *pTimeout)
{
	int recvLen = 0;
	fd_set rset;

	if(pTimeout) {
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		if(select(fd+1, &rset, NULL, NULL, pTimeout) <= 0) {
			perror("select failed");
			return -1;
		}
		
		if(FD_ISSET(fd, &rset)) {
			if((recvLen = recv(fd, buf, len, 0)) <= 0) {
				perror("recv");
				return -1;
	    	}
		}
	}else {
		if((recvLen = recv(fd, buf, len, 0)) <= 0) {
			perror("recv");
			return -1;
	    }
	}
    
    return recvLen;
}























