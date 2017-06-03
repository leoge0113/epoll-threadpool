/*
 * config_info.h
 *
 *  Created on: 2016/11
 *      Author: Leo
 */

#ifndef CONFIG_INFO_H_
#define CONFIG_INFO_H_
#include "g_net_global.h"

#define FILE_NAME_LEN				128
#define SIZE_OF_VERSION_DATA_BUF	64
#define PATH_BUFF_SIZE				128
#define DB_NAME_SIZE				64
#define DB_USER_NAME_SIZE			64
#define DB_PASSWORD_NAME_SIZE		128

#define CONFIG_KEY_WORD_BUF_SIZE	128
#define INI_FILE_BUF_SIZE			(10 * 1024)

#define SERVER_INI_FILE				"ini.ini"


enum
{
	CON_VERSION,
	CON_LOG_LEVEL,
	CON_MYSQL_BRANCH_SERVER_ADDR,
	CON_MYSQL_SERVER_PORT,
	CON_MYSQL_DB_NAME,
	CON_MYSQL_USER_NAME,
	CON_MYSQL_PASSWORD,
	MAX_NO_OF_CONFIG_KEY_WORD,
} CONFIG_INFO_KEY_WORD_ID;	//Should correspond with structure CONFIG_INFO.

typedef struct _CONIFG_INFO
{
	char 				version[32];
	char 				log_level;
	char 				mysql_branch_server_addr[PATH_BUFF_SIZE];
	unsigned int 		mysql_server_port;
	char 				mysql_db_name[DB_NAME_SIZE];
	char 				mysql_user_name[DB_USER_NAME_SIZE];
	char 				mysql_password[DB_PASSWORD_NAME_SIZE];
}CONFIG_INFO;	//Should correspond with structure CONFIG_INFO_KEY_WORD_ID and CONFIG_INFO.

int read_config_info(CONFIG_INFO *config_info);
void print_config_info(CONFIG_INFO config_info);
#endif /* _CONIFG_INFO */




