/*
 * g_net_update.c
 *
 *  Created on:  2016.11
 *      Author: leo
 */
/********************** g_net update version 2.0 ***************************/
#include "config_info.h"
#include "log.h"
#include "epoll_connect.h"
#include "thread_pool.h"
#include "g_net_update.h"
#include "database_process.h"


#define	CONNECT_TO_SQL_SUCCESS							0
#define SERVER_TIMEOUT									60 * 1 //60S

static CONFIG_INFO config_info;
static int epoll_fd = -1; // the epoll fd
static int listen_fd = -1; // the socket fd socket create return
static pthread_t accep_thread_t;
static pthread_t send_thread_t;

static int current_connected_total = 0; // the connections number
static int exit_accept_flag = 0; // is exit the accept
static int exit_flag = 0; // is exit the epoll wait
static int port = 8111;

static void closesocket(int fd);
static void dumpInfo(unsigned char *info, int length);
pthread_mutex_t connect_total_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static void connect_total_count_opration(BOOL add_or_subract, int value)
{
	pthread_mutex_lock(&connect_total_count_mutex);
	if (add_or_subract)
	{
		current_connected_total = current_connected_total + value;
	}
	else
	{
		current_connected_total = current_connected_total - value;
	}
	pthread_mutex_unlock(&connect_total_count_mutex);
}

static void signal_handler_reboot(int32_t theSignal)
{
	int i;
	int sockfd;
	char log_str_buf[LOG_STR_BUF_LEN];
	signal(SIGPIPE, SIG_IGN);
	if (SIGKILL == theSignal || SIGTERM == theSignal) //we can know when system excute child thread is end
	{
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "receive kill signal exit the server.");
		LOG_INFO(LOG_LEVEL_FATAL, log_str_buf);
		if (listen_fd != -1)
		{
			closesocket(listen_fd);
			listen_fd = -1;
		}
		exit_flag = 1;
		exit_accept_flag = 1;
		for (i = 0; i < MAX_EVENTS; i++)
		{
			sockfd = get_fd_by_event_index(i);
			if (sockfd != -1)
			{
				closesocket(sockfd);
			}
		}
#if CONNECT_TO_SQL_SUCCESS
	sql_pool_destroy();
#endif
	}
	else if (SIGPIPE == theSignal)
	{
		LOG_INFO(LOG_LEVEL_INFO, "SIGPIPE received.\n");
	}
}

static void closesocket(int fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}

static int set_non_blocking(int sock)
{
	int opts = -1;
	char log_str_buf[LOG_STR_BUF_LEN];

	opts = fcntl(sock, F_GETFL);
	if (opts < 0)
	{
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "fcntl(sock=%d,GETFL).\n", sock);
		LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
		return (-1);
	}
	opts = opts | O_NONBLOCK;
	if (fcntl(sock, F_SETFL, opts) < 0)
	{
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "fcntl(sock=%d,GETFL).\n", sock);
		LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
		return (-1);
	}
	return 1;
}

CONFIG_INFO *get_config_info(void)
{
	return &config_info;
}
/****************************** accept task ******************************************/
static void *accept_thread(void *arg)
{
	int connect_fd = -1;
	socklen_t clilen = 0;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	struct sockaddr cliaddr;
	struct epoll_event ev;
	int val = 1;
	int err = 0;
	int bufsize = 32 * 1024; //1024*2048;
	int epoll_connect_event_index = -1;
	char log_str_buf[LOG_STR_BUF_LEN] = {0};

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == listen_fd) // INVALID_SOCKET
	{
		LOG_INFO(LOG_LEVEL_ERROR, "create socket error.\n");
		return NULL;
	}
	//set socket options
	err = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	if (0 != err)
	{
		LOG_INFO(LOG_LEVEL_ERROR, "setsockopt SO_REUSEADDR error.\n");
		return NULL;
	}
	err = setsockopt(listen_fd, SOL_SOCKET, SO_RCVBUF, (char*)(&bufsize), sizeof(int));
	if (0 != err)
	{
		LOG_INFO(LOG_LEVEL_ERROR, "setsockopt SO_RCVBUF error.\n");
		return NULL;
	}

	val = 2; //2 minitues
	err = setsockopt(listen_fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val));
	if (0 != err)
	{
		LOG_INFO(LOG_LEVEL_ERROR, "setsockopt TCP_DEFER_ACCEPT error.\n");
		return NULL;
	}

//	set_non_blocking(listen_fd);
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(/*config_info.port*/port); //serveraddr.sin_port  = htons(SERVER_LISTEN_PORT);
	serveraddr.sin_family = AF_INET;
	if (-1 == bind(listen_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) // SOCKET_ERROR
	{
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "bind config port %d error.\n", /*config_info.port*/port);
		LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
		printf("bind config port %d error.\n", /*config_info.port*/port);
		return NULL;
	}
	printf("bind config port %d success.\n", /*config_info.port*/port);

	listen(listen_fd, LISTENQ); // 4096
	clilen = sizeof(cliaddr);
	while (!exit_accept_flag)
	{
		if (current_connected_total < MAX_EVENTS) // effective
		{
			if ((connect_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &clilen)) < 0)
			{
				if ((errno != EAGAIN) && (errno != ECONNABORTED) && (errno != EPROTO) && (errno != EINTR))
				{
					snprintf(log_str_buf, LOG_STR_BUF_LEN, "accept error %d,%s.\n", errno, strerror(errno));
					LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
				}
				continue;
			}
		}
		else
		{
			sleep(1);
			snprintf(log_str_buf, LOG_STR_BUF_LEN, "current accept number achieve to maximum(%d) .\n", MAX_EVENTS);
			LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
			continue;
		}
		epoll_connect_event_index = get_epoll_connect_free_event_index();
		// no free epoll event
		if (epoll_connect_event_index == -1) // no the free connect event
		{
			LOG_INFO(LOG_LEVEL_ERROR, "Not find free connect.\n");
			if (connect_fd != -1)
			{
				closesocket(connect_fd);
				connect_fd = -1;
			}
			continue;
		}
		// set connect fd non blocking
		if (set_non_blocking(connect_fd) < 0)
		{
			snprintf(log_str_buf, LOG_STR_BUF_LEN, "set non-blocking socket = %d error.\n", connect_fd);
			LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
			if (connect_fd != -1)
			{
				closesocket(connect_fd);
				connect_fd = -1;
			}
			continue;
		}
		err = setsockopt(connect_fd, SOL_SOCKET, SO_RCVBUF, (char *)(&bufsize), sizeof(int));
		if (0 != err)
		{
			snprintf(log_str_buf, LOG_STR_BUF_LEN, "set socket(%d) setsockopt SO_RCVBUF error.\n", connect_fd);
			LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
			if (connect_fd != -1)
			{
				closesocket(connect_fd);
				connect_fd = -1;
			}
			continue;
		}
		init_epoll_connect_by_index(epoll_connect_event_index, connect_fd, inet_ntoa(clientaddr.sin_addr));
		// add epoll event
		ev.data.fd = connect_fd;
		ev.events = EPOLLIN | EPOLLET; // set epoll event type
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &ev) == -1)
		{
			snprintf(log_str_buf, LOG_STR_BUF_LEN, "EPOLL_CTL_ADD %d,%s.\n", errno, strerror(errno));
			LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
			if (connect_fd != -1)
			{
				closesocket(connect_fd);
				connect_fd = -1;
			}
			continue;
		}
		connect_total_count_opration(TRUE, 1);
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "Epoll event[%d] *****connected from %s*****, fd:%d, current connect total = %d.\n", epoll_connect_event_index,
				        inet_ntoa(clientaddr.sin_addr), connect_fd, current_connected_total);
		LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
	}
	if (-1 != listen_fd) // out the while then close the socket
	{
		closesocket(listen_fd);
		listen_fd = -1;
	}
	return NULL;
}

static int create_accept_task(void)
{
	return pthread_create(&accep_thread_t, NULL, accept_thread, NULL);
}
/*************************************************************************/

static int recv_buffer_from_fd(int socket_fd, char *recv_buffer, int *recv_length)
{
	int rev = -1;
	int one_time_need_read = 1024;
	int continue_read = 1;
	int total_recv_lenght = 0, one_time_recv_lenght = 0;
	while (continue_read)
	{
		one_time_recv_lenght = recv(socket_fd, &recv_buffer[total_recv_lenght], one_time_need_read, 0);
		if (one_time_recv_lenght > 0)
		{
			total_recv_lenght += one_time_recv_lenght;
		}

		if (one_time_recv_lenght < 0) // error
		{
//			if (errno == EAGAIN)
//			{
//				continue_read = 0;
//				break;
//			}
			continue_read = 0;
			rev = -1;
			break;
		}
		else if (one_time_recv_lenght >= 0 && one_time_recv_lenght < one_time_need_read) // read over
		{
			continue_read = 0;
			rev = 0;
			*recv_length = total_recv_lenght;
			break;
		}
		else
		{
			continue_read = 0;
			rev = -1;
			break;
		}
	}
	return rev;
}


static int send_buffer_to_fd(int socket, unsigned char *pucBuf, int iLen)
{
	int n;
	int nwrite;
	char log_str_buf[LOG_STR_BUF_LEN];

	n = iLen;

	while (n > 0)
	{
		nwrite = write(socket, pucBuf + iLen - n, n);
		if (nwrite < n)
		{
			if (nwrite == -1)
			{
				if (errno != EAGAIN)
				{
					snprintf(log_str_buf, LOG_STR_BUF_LEN, "Write.\n");
					LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
					return 0;
				}
				else
				{
					snprintf(log_str_buf, LOG_STR_BUF_LEN, "Write EAGAIN.\n");
					LOG_INFO(LOG_LEVEL_INFO, log_str_buf);
				}
			}
		}
		n -= nwrite;
	}
	return (iLen - n);
}


static void dumpInfo(unsigned char *info, int length)
{
	char log_str_buf[LOG_STR_BUF_LEN];
	int index = 0;
	int j = 0;
	char buffer[128] = {0};
	memset(buffer, 0, 128);
	for (index = 0; index < length; index++, j+=2)
	{
		sprintf(&buffer[j],"%02x",info[index]);
	}
	snprintf(log_str_buf, LOG_STR_BUF_LEN, "dump info length = %d, %s\n", length, buffer);
	LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
}


void* respons_stb_info(thpool_job_funcion_parameter *parameter, int thread_index)
{
	char log_str_buf[LOG_STR_BUF_LEN];
	char send_buffer[1024] = "i have get you data";
	int sockfd = parameter->fd;
	printf("get buffer = %s\n", parameter->recv_buffer);
	printf("sockfd = %d\n", sockfd);
	send_buffer_to_fd(sockfd, send_buffer, strlen(send_buffer));
	// deal with you logic
	if (sockfd != -1)
	{
		int mactched_event_index = get_matched_event_index_by_fd(sockfd);
		connect_total_count_opration(FALSE, 1);
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "send data to stb over then close the socket(%d), *****client addr(%s)***** , thread_index = %d.\n", sockfd, get_client_addr_by_index(mactched_event_index), thread_index);
		LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
		free_event_by_index(mactched_event_index);
		closesocket(sockfd);
		sockfd = -1;
	}

}
/*******************************************************************/

int main(int argc, char *argv[])
{
	char log_file_name[128] = {0};
	char log_str_buf[LOG_STR_BUF_LEN];
	struct rlimit rl;
	thpool_t *thpool = NULL;
	time_t now;
	time_t prevTime, eventTime = 0;
	struct epoll_event ev, events[MAX_EVENTS];
	int epoll_events_number = 0;
	int index = 0;
	int connect_socket_fd_temp = -1;
	int delete_pool_job_number = 0;

	if (2 != argc)
	{
		printf("pleanse input the port\n");
		return 0;
	}
	port = atoi(argv[1]);
	// read config info
	if (read_config_info(&config_info) != 0)
	{
		printf("[%s %s %d] read_config_info fail.\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	print_config_info(config_info);

	signal(SIGCHLD, SIG_IGN); // Ignore the child to the end of the signal, preventing the zombie process(2015.7.17)

	// init log
	sprintf(log_file_name, "log_%d", port);
	// for distinguish between different ports
	set_log_file_name(log_file_name);
	if (log_init() != 0)
	{
		printf("init log error\n");
		return -1;
	}
	log_set_level(config_info.log_level);

	// create fork
	pid_t pidfd = fork();
	if (pidfd < 0)
	{
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "fork failed! errorcode = %d[%s].\n", errno, strerror(errno));
		LOG_INFO(LOG_LEVEL_FATAL, log_str_buf);
		log_close();
		return (-1);
	}
	if (pidfd != 0)
	{
		LOG_INFO(LOG_LEVEL_INFO, "parent fork over.\n");
		exit(0);
	}
	setsid();
	LOG_INFO(LOG_LEVEL_INFO, "children fork start.\n");

	//set max number of open files(also for tcp connections)
	rl.rlim_cur = MAX_EVENTS;
	rl.rlim_max = MAX_EVENTS;
	setrlimit(RLIMIT_NOFILE, &rl);
	getrlimit(RLIMIT_NOFILE, &rl);
	fprintf(stderr, "cur:%d\n", (int)rl.rlim_cur);
	fprintf(stderr, "max:%d\n", (int)rl.rlim_max);
	snprintf(log_str_buf, LOG_STR_BUF_LEN, "information about rlimit cur:%d max:%d.\n", (int)rl.rlim_cur, (int)rl.rlim_max);
	LOG_INFO(LOG_LEVEL_INFO, log_str_buf);

	sleep(1);
	signal(SIGKILL, signal_handler_reboot); // register the signal
	signal(SIGTERM, signal_handler_reboot);
	signal(SIGPIPE, signal_handler_reboot);

	init_epoll_connect();
//	g_net_init_mysql(); // connect to the database
#if CONNECT_TO_SQL_SUCCESS
	if (0 != sql_pool_create(THREAD_POLL_SIZE))
	{
		LOG_INFO(LOG_LEVEL_FATAL, "mysql error.\n");
		log_close();
		return -1;
	}
#endif
	epoll_fd = epoll_create(MAX_FDS); // 1024
	if (0 >= epoll_fd)
	{
		LOG_INFO(LOG_LEVEL_FATAL, "epoll_create error.\n");
		log_close();
		return -1;
	}

	// crate thread pool
	thpool = thpool_init(THREAD_POLL_SIZE);
	if (NULL == thpool)
	{
		LOG_INFO(LOG_LEVEL_FATAL, "create thread pool error.\n");
		log_close();
		return -1;
	}
	LOG_INFO(LOG_LEVEL_INDISPENSABLE, "thpool_init success.\n");

	// accept the socket connect
	create_accept_task();

	time(&prevTime);
	eventTime = prevTime;
	while (!exit_flag)
	{
		time(&now);
		if (abs(now - eventTime) >= SERVER_TIMEOUT) //SERVER_TIMEOUT second detect one time delete the time out event
		{
			eventTime = now;
			for (index = 0; index < MAX_EVENTS; index++)
			{
				connect_socket_fd_temp = get_fd_by_event_index(index);
				if (connect_socket_fd_temp != -1)
				{
					if ((now - get_event_connect_time_by_index(index)) > SERVER_TIMEOUT)
					{
						snprintf(log_str_buf, LOG_STR_BUF_LEN, "Epoll event[%d] timeout closed and fd= %d.\n", index, connect_socket_fd_temp);
						LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
						free_event_by_index(index);
						if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_socket_fd_temp, &ev) == -1)
						{
							snprintf(log_str_buf, LOG_STR_BUF_LEN, "EPOLL_CTL_DEL %d,%s.\n", errno, strerror(errno));
							LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
						}
						connect_total_count_opration(FALSE, 1);
						closesocket(connect_socket_fd_temp);
						connect_socket_fd_temp = -1;
					}
				}
			}
			// delete the pool job time out job
			delete_pool_job_number = delete_timeout_job(thpool, SERVER_TIMEOUT);
			connect_total_count_opration(FALSE, delete_pool_job_number);
			snprintf(log_str_buf, LOG_STR_BUF_LEN, "pool queque delete job number = %d.\n", delete_pool_job_number);
			LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
		}
		epoll_events_number = epoll_wait(epoll_fd, events, MAX_EVENTS, 2000); //2seconds
		for (index = 0; index < epoll_events_number; ++index) // deal with the event
		{
			connect_socket_fd_temp = events[index].data.fd; // get the socket fd
			// delete epoll event
			if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[index].data.fd, &ev) == -1)
			{
				snprintf(log_str_buf, LOG_STR_BUF_LEN, "EPOLL_CTL_DEL %d,%s.\n", errno, strerror(errno));
				LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
				events[index].data.fd = -1;
			}
			if (events[index].events & EPOLLIN) //have read event
			{
				int event_index = -1;
				int recv_length = 0;
				unsigned char recv_buffer[BUFFER_SIZE];

				if (connect_socket_fd_temp < 0)
				{
					connect_total_count_opration(FALSE, 1);
					snprintf(log_str_buf, LOG_STR_BUF_LEN, "Event[%d] read invalid handle.\n", index);
					LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
					continue;
				}
				event_index = get_matched_event_index_by_fd(connect_socket_fd_temp);
				snprintf(log_str_buf, LOG_STR_BUF_LEN, "Epoll get Event[%d] fd = %d.\n", event_index, connect_socket_fd_temp);
				LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
				// no the event
				if (event_index < 0)
				{
					connect_total_count_opration(FALSE, 1);
					snprintf(log_str_buf, LOG_STR_BUF_LEN, "not find matched fd = %d.\n", connect_socket_fd_temp);
					LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
					free_event_by_index(event_index);
					if (connect_socket_fd_temp != -1)
					{
						closesocket(connect_socket_fd_temp);
						connect_socket_fd_temp = -1;
					}
					continue;
				}
				// receive the buffer from the socket fd
				if (0 == recv_buffer_from_fd(connect_socket_fd_temp, recv_buffer, &recv_length))
				{
					snprintf(log_str_buf, LOG_STR_BUF_LEN, "recv_length = %d, current fd = %d, current job queue job number = %d.\n",recv_length, connect_socket_fd_temp, get_jobqueue_number(thpool));
					LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
					dumpInfo((unsigned char *)recv_buffer, recv_length);
					// receive buffer success then add the thread pool
					thpool_add_work(thpool, (void*)respons_stb_info, connect_socket_fd_temp, recv_buffer);
				}
				else
				{
					// receive buffer error
					connect_total_count_opration(FALSE, 1);
					snprintf(log_str_buf, LOG_STR_BUF_LEN, "Epoll event[%d] not read data, and the socket fd = %d.\n", event_index, connect_socket_fd_temp);
					LOG_INFO(LOG_LEVEL_INDISPENSABLE, log_str_buf);
					free_event_by_index(event_index);
					if (connect_socket_fd_temp != -1)
					{
						closesocket(connect_socket_fd_temp);
						connect_socket_fd_temp = -1;
					}
				}
			}
//			else if (events[index].events & EPOLLOUT) //have read event. Will not reach here
//			{
//				//;
//			}
			else
			{
				snprintf(log_str_buf, LOG_STR_BUF_LEN, "Unknown error! event.data.fd = %d.\n", events[index].data.fd);
				LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
				connect_total_count_opration(FALSE, 1);
				if (connect_socket_fd_temp < 0)
				{
					LOG_INFO(LOG_LEVEL_ERROR, "EPOLLOUT.\n");
					continue;
				}
				//close the socket
				free_event_by_index(get_matched_event_index_by_fd(connect_socket_fd_temp));
				if (connect_socket_fd_temp != -1)
				{
					closesocket(connect_socket_fd_temp);
					connect_socket_fd_temp = -1;
				}
			}
		}
	}
	log_close();
	if (listen_fd != -1)
	{
		closesocket(listen_fd);
		listen_fd = -1;
	}

#if CONNECT_TO_SQL_SUCCESS
	sql_pool_destroy();
#endif
	exit_accept_flag = 1;
	thpool_destroy(thpool);
	printf("[%s %s %d]Done...\n", __FILE__, __FUNCTION__, __LINE__);
	return 1;
}


