/*
 * sql_pool.h
 *
 *  Created on: 201611
 *      Author: leo
 */

#ifndef SQL_POOL_H_
#define SQL_POOL_H_
#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <sys/socket.h>
#include <netdb.h>
#include "g_net_global.h"

#define DB_LEN_IP			15
#define DB_LEN_PORT			8
#define DB_LEN_DBNAME		64
#define DB_LEN_DBUSER		64
#define DB_LEN_PASSWD		64

#define POOL_NUMBER			20

typedef struct _SQL_SOCK_NODE{
    MYSQL						fd;
    MYSQL 						*mysql_sock;
    pthread_mutex_t 			sql_lock;
    char						used;
    int 						index; // for test
    enum {DB_DISCONN, DB_CONN} 	sql_state;
} SQL_SOCK_NODE;

typedef struct _POOL_SQL_SOCK{
	SQL_SOCK_NODE sql_pool[POOL_NUMBER];

    char ip[DB_LEN_IP + 1];
	int  port;
	char dbname[DB_LEN_DBNAME + 1];
	char user[DB_LEN_DBUSER + 1];
	char passwd[DB_LEN_PASSWD + 1];

    int pool_number;
} POOL_SQL_SOCK;

void sql_pool_destroy();
SQL_SOCK_NODE *get_db_connect_from_pool();
void release_sock_to_sql_pool(SQL_SOCK_NODE *n);
void check_sql_sock_normal(SQL_SOCK_NODE *n);
int sql_pool_create(int connect_pool_number);



#endif /* SQL_POOL_H_ */
