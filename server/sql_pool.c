/*
 * sql_pool.c
 *
 *  Created on: 201611
 *      Author: leo
 */
#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <sys/socket.h>
#include <netdb.h>

#include "g_net_global.h"
#include "config_info.h"
#include "log.h"
#include "g_net_update.h"
#include "thread_pool.h"
#include "sql_pool.h"


static POOL_SQL_SOCK sql_sock_pool;

// return 0: connect success, 1 connect error, -1: init error
static int create_connect(POOL_SQL_SOCK *pool_sql_sock, SQL_SOCK_NODE *node)
{
	int ret = 0;
	int opt = 1;
	char log_str_buf[LOG_STR_BUF_LEN] = {0};

	do
	{
		if (NULL == mysql_init(&node->fd))
		{
			snprintf(log_str_buf, LOG_STR_BUF_LEN, "mysql_init error!.\n");
			LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
			ret = -1;
		}
		else
		{
			if(pthread_mutex_init(&node->sql_lock, NULL))
			{
				snprintf(log_str_buf, LOG_STR_BUF_LEN, "Couldn't pthread_mutex_init to engine!.\n");
				LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
			}

			if(!(node->mysql_sock = mysql_real_connect(&node->fd, pool_sql_sock->ip, pool_sql_sock->user, pool_sql_sock->passwd, pool_sql_sock->dbname,
					pool_sql_sock->port, NULL, 0)))
			{
				snprintf(log_str_buf, LOG_STR_BUF_LEN, "Couldn't connect to engine! %s.\n", mysql_error(&node->fd));
				LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
				node->sql_state = DB_DISCONN;
				ret = 1;
				printf("connect error\n");
			}
			else
			{
				node->used = 0;
				node->sql_state = DB_CONN;
				mysql_options(&node->fd, MYSQL_OPT_RECONNECT, &opt);
				opt = 3; //3S
				mysql_options(&node->fd, MYSQL_OPT_CONNECT_TIMEOUT, &opt);
				ret = 0;
			}
		}

	}while(0);

	return ret;
}

int sql_pool_create(int connect_pool_number)
{
    int index = 0;
    BOOL re_connect = TRUE;
    MYSQL mysql;
    MYSQL fd_temp;
    char log_str_buf[LOG_STR_BUF_LEN] = {0};
    CONFIG_INFO *config = (CONFIG_INFO *)get_config_info();

    // init
    memset((POOL_SQL_SOCK *)&sql_sock_pool, 0, sizeof(POOL_SQL_SOCK));

    // set db info
    sprintf(sql_sock_pool.ip, "%s", config->mysql_branch_server_addr);
    sql_sock_pool.port = config->mysql_server_port;
    sprintf(sql_sock_pool.dbname, "%s", config->mysql_db_name);
    sprintf(sql_sock_pool.user, "%s", config->mysql_user_name);
    sprintf(sql_sock_pool.passwd, "%s", config->mysql_password);

    mysql_init(&fd_temp);

    // create connect
    for(index = 0; index < connect_pool_number; index ++)
    {
		if(-1 == create_connect(&sql_sock_pool, &(sql_sock_pool.sql_pool[index])))
		{
			goto POOL_CREATE_FAILED;
		}
		printf("create db pool success\n");
		sql_sock_pool.sql_pool[index].index = index;
        sql_sock_pool.pool_number++;
    }
    return 0;
POOL_CREATE_FAILED:
	sql_pool_destroy();
    return -1;
}

void sql_pool_destroy()
{
	int index;
	for (index = 0; index < sql_sock_pool.pool_number; index ++)
	{
		if (NULL != sql_sock_pool.sql_pool[index].mysql_sock) // close the mysql
		{
			mysql_close(sql_sock_pool.sql_pool[index].mysql_sock); // close
			sql_sock_pool.sql_pool[index].mysql_sock = NULL;
		}
		sql_sock_pool.sql_pool[index].sql_state = DB_DISCONN;
	}
}


SQL_SOCK_NODE *get_db_connect_from_pool()
{
	char log_str_buf[LOG_STR_BUF_LEN] = {0};
	int start_index = 0, loop_index = 0, index = 0, ping_ret = 0;

	srand((int) time(0));
	start_index = rand() % sql_sock_pool.pool_number;

	for (loop_index = 0; loop_index < sql_sock_pool.pool_number; loop_index++)
	{
		index = (start_index + loop_index) % sql_sock_pool.pool_number;
		if (0 == pthread_mutex_trylock(&(sql_sock_pool.sql_pool[index].sql_lock)))
		{
			if (DB_DISCONN == sql_sock_pool.sql_pool[index].sql_state)
			{
				// try reconnect
				if (0 != create_connect(&sql_sock_pool, &(sql_sock_pool.sql_pool[index])))
        		{
					// also can not connect to the database
        			release_sock_to_sql_pool(&(sql_sock_pool.sql_pool[index]));
        			continue;
        		}
			}
			ping_ret = mysql_ping(sql_sock_pool.sql_pool[index].mysql_sock);
			if (0 != ping_ret)
			{
				snprintf(log_str_buf, LOG_STR_BUF_LEN, "mysql_ping error!.\n");
				LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
				// error
				sql_sock_pool.sql_pool[index].sql_state = DB_DISCONN; // ping error then next time reconnect
				release_sock_to_sql_pool(&(sql_sock_pool.sql_pool[index]));
			}
			else
			{
				sql_sock_pool.sql_pool[index].used = 1;
				break;
			}
		}

	}
	if (loop_index == sql_sock_pool.pool_number)
	{
		return NULL;
	}
	else
	{
		return &(sql_sock_pool.sql_pool[index]);
	}

}

// put to pool
void release_sock_to_sql_pool(SQL_SOCK_NODE * n)
{
	n->used = 0;
    pthread_mutex_unlock(&n->sql_lock);
}

// error
void check_sql_sock_normal(SQL_SOCK_NODE * n)
{
    n->sql_state = DB_DISCONN;
}
