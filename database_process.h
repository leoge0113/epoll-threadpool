/*
 * database_process.h
 *
 *  Created on: 20161.11
 *      Author: LEO
 */

#ifndef DATABASE_PROCESS_H_
#define DATABASE_PROCESS_H_
#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <sys/socket.h>
#include <netdb.h>

#define ENABLE_DB_POOL			1

typedef struct
{
	BOOL is_mysql_init; // may be when server start cann't connect to database
	MYSQL mysql;
	MYSQL *mysql_sock;

} MYSQL_CONNECT_INFO;

int get_upgrade_count_from_database ();


#endif /* DATABASE_PROCESS_H_ */
