#ifndef INCLUDE_NETWORK_H
#define INCLUDE_NETWORK_H

#include "comdef.h"
#include <netinet/in.h>

enum net_t
{
    UDP = 0,
    TCP = 1,
};

struct net_param
{
	enum net_t type;		//UDP or TCP
	char * serip;			//server ip, eg: "127.0.0.1"
	int serport;			//server port, eg: 8000
};

struct net_handle
{
	int sktfd;
	int sersock_len;
	struct sockaddr_in server_sock;
	struct net_param params;
};

extern int getClientNum(void);
struct net_handle *net_open(struct net_param params);
int net_send(struct net_handle *handle, void *data, int size, int type);
int net_recv(struct net_handle *handle, void *data, int size);
void net_close(struct net_handle *handle);

#endif
