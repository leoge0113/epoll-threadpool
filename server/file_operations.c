/*
 * file_operations.c
 *
 *  Created on: 2016.11
 *      Author: Leo
 */

#include "log.h"
#include "file_operations.h"

int read_file_to_buff(char *file_path, unsigned long len, char *data)
{
	int ret = -1;
	FILE *fp = NULL;
	int count = -1;

	if ((fp = fopen(file_path, "r")) == NULL)
	{
		char log_str_buf[LOG_STR_BUF_LEN];
		snprintf(log_str_buf, LOG_STR_BUF_LEN, "[%s %s %d] Can't open %s.\n", __FILE__, __FUNCTION__, __LINE__, file_path);
		log_string(LOG_LEVEL_ERROR, log_str_buf);
		return -1;
	}

	if ((count = fread(data, 1, len, fp)) > 0)
	{
		ret = 0;
	}

	if (fp != NULL)
	{
		fclose(fp);
	}

	return ret;
}
