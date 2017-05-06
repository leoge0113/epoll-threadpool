/*
 * g_net_global.h
 *
 *  Created on: 2016.11
 *      Author: Leo
 */

#ifndef G_NET_GLOBAL_H_
#define G_NET_GLOBAL_H_


#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
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
#include <sched.h>

#include <sys/socket.h>
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

#include <poll.h>
#include <sys/syscall.h>
#include <pthread.h>


#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

typedef int BOOL;


#ifndef FALSE
#define	FALSE						(0)
#endif
#ifndef	TRUE
#define	TRUE						(!FALSE)
#endif

#define MIN(a, b)					((a)<(b)?(a):(b))


#endif /* G_NET_GLOBAL_H_ */
