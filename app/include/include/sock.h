#ifndef __SOCK_H
#define __SOCK_H

#include "comdef.h"
#include <netinet/in.h>

int socket_udp(char *ip, short port);
int socket_udpRecv(int fd, char *buf, int len, struct sockaddr_in *fromAddr);
int socket_udpSend(int fd, char *buf, int len, struct sockaddr_in *toAddr);

int socket_tcp(char *ip, short port);
int socket_tcpListen(int sockfd, int num);
int socket_tcpAccept(int sockfd, struct sockaddr_in *clientAddr);
int socket_tcpConnect(char *ip, short port);
int socket_tcpSend(int fd, void *buf, u32 len, struct timeval *pTimeout);
int socket_tcpRecv(int fd, char *buf, int len, struct timeval *pTimeout);

#endif
