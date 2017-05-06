/*
 * thread_pool_global.h
 *
 *  Created on: 
 *      Author: leo
 */

/**********************************
 * @author     <a href="mailto:wallwind@yeah.net">wallwind@<span style="color:#000000;">yeah.net</span></a>
 * @date        2012/06/13
 * Last update: 2012/06/13
 * License:     LGPL
 *
 **********************************/

#ifndef _THREAD_POOL_GLOBAL_H_
#define _THREAD_POOL_GLOBAL_H_

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>             /* */
#include <stdarg.h>
#include <stddef.h>             /* offsetof() */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <glob.h>
#include <sys/vfs.h>            /* statfs() */

#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>        /* TCP_NODELAY, TCP_CORK */
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#include <time.h>               /* tzset() */
#include <malloc.h>             /* memalign() */
#include <limits.h>             /* IOV_MAX */
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <crypt.h>
#include <sys/utsname.h>        /* uname() */
#include <semaphore.h>

#include <sys/epoll.h>
#include <poll.h>
#include <sys/syscall.h>
#include <pthread.h>
#endif




