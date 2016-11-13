/*
 * database_process.c
 *
 *  Created on: 11.13 2014
 *      Author: LEO
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
#include "config_info.h"
#include "database_process.h"

#define MYSQL_SELECT_UPGRADE_COUNT_STATEMENT "select sw_count from cust_sw_ver where (sw_from & 1) and sw_public = 1 and pid = %d and cid = %d and mid = %d \
	and sw_ver = %d"
#define MYSQL_SELECT_UPGRADE_COUNT_STATEMENT_TEST "select sw_count from cust_sw_ver where (sw_from & 1) and sw_public = 1 and pid = 5 and cid = 20 and mid = 9 \
	and sw_ver = 172"

// select the upgrade count from database
int get_upgrade_count_from_database ()
{
	MYSQL_RES *res;
	MYSQL_FIELD *fd ;
	MYSQL_ROW row ;
	SQL_SOCK_NODE *socket_node = NULL;
	char log_str_buf[LOG_STR_BUF_LEN];
	int upgrade_count = -1; // -1: mean no find the upgrade count

	socket_node = get_db_connect_from_pool();
	if (NULL == socket_node)
	{
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "Get socket node error.\n");
		LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
		return upgrade_count;
	}
	if(mysql_query(&(socket_node->fd), MYSQL_SELECT_UPGRADE_COUNT_STATEMENT_TEST))
	{
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "Query failed (%s).\n", mysql_error(&(socket_node->fd)));
		LOG_INFO(LOG_LEVEL_ERROR, log_str_buf);
		goto EXIT;
	}

	if (!(res=mysql_store_result(&(socket_node->fd))))
	{
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "Couldn't get result from %s\n", mysql_error(&(socket_node->fd)));
		LOG_INFO(LOG_LEVEL_INFO, log_str_buf);
		goto EXIT;
	}

	if ((row = mysql_fetch_row(res)) != NULL)
	{
		upgrade_count = atoi(row[0]);
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "Get the count success count is %d.\n", upgrade_count);
		LOG_INFO(LOG_LEVEL_INFO, log_str_buf);
	}
	else
	{
		LOG_INFO(LOG_LEVEL_INFO, "Get the count fail!\n");
	}
	mysql_free_result(res);
EXIT:
	release_sock_to_sql_pool(socket_node);
	return upgrade_count;
}


