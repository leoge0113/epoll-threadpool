/*
 * epoll_connect.h
 *
 *  Created on: 
 *      Author: leo
 */

#ifndef EPOLL_CONNECT_H_
#define EPOLL_CONNECT_H_
#include "g_net_global.h"
#define MAX_FDS            10240 //10240
#define MAX_EVENTS         MAX_FDS
#define LISTENQ            4096
#define	IP_ADDR_LENGTH	   20

typedef struct _epoll_connect_struct_
{
	int connect_fd;
	int socket_status; //0--initial,1--Live,2--need send,3--Need Close
	time_t now;
	/*********************/
	/*The Client IP address and PORT : TCP*/
	char client_ip_addr[IP_ADDR_LENGTH];
	pthread_mutex_t mutex;
} EPOLL_CONNECT;

void init_epoll_connect(void);
int get_epoll_connect_free_event_index(void);
int get_matched_event_index_by_fd(int iConnectFD);
void free_event_by_index(int index);
int get_fd_by_event_index(int index);
time_t get_event_connect_time_by_index(int index);
char *get_client_addr_by_index(int index);

#endif /* EPOLL_CONNECT_H_ */
