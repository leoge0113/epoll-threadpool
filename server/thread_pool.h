/*
 * thread_pool.h
 *
 *  Created on: 2015Äê2ÔÂ4ÈÕ
 *      Author: vae
 */

#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include "thread_pool_global.h"

#define	  	THREAD_POLL_SIZE	 	15// the thread number of thread poll
#define		BUFFER_SIZE				1024

typedef void* (*FUNC)(void* arg, int index);


typedef struct _thpool_job_funcion_parameter{
	char recv_buffer[1024];
	int  fd;
}thpool_job_funcion_parameter;

/**
 * define a task note
 */
typedef struct _thpool_job_t{
    FUNC             		function;		// function point
//    void*                 	arg;     		// function parameter
    thpool_job_funcion_parameter arg;
    time_t job_add_time;
    struct _thpool_job_t* 	prev;     		// point previous note
    struct _thpool_job_t* 	next;     		// point next note
}thpool_job_t;

/**
 * define a job queue
 */
typedef struct _thpool_job_queue{
   thpool_job_t*    head;            	//queue head point
   thpool_job_t*    tail;             	//queue tail point
   int              jobN;               //task number
   sem_t*           queueSem;           //queue semaphore
}thpool_jobqueue;

/**
 * thread pool
 */
typedef struct _thpool_t{
   pthread_t*      	threads;    // thread point
   int             	threadsN;   // thread pool number
   thpool_jobqueue* jobqueue;  	// job queue
}thpool_t;


typedef struct _thpool_thread_parameter{
	thpool_t 		*thpool;
	int 			thread_index;
}thpool_thread_parameter;


thpool_t*  thpool_init(int thread_pool_numbers);
int thpool_add_work(thpool_t* tp_p, void *(*function_p)(void*arg, int index), /*void *arg_p*/int socket_fd, char *recev_buffer);
void thpool_destroy(thpool_t* tp_p);
int get_jobqueue_number(thpool_t* tp_p);
int delete_timeout_job(thpool_t* tp_p, int time_out);

#endif /* THREAD_POOL_H_ */
