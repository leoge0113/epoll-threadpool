/*
 * thread_pool.c
 *
 *  Created on: 201610
 *      Author: leo
 */
#include "thread_pool_global.h"
#include "thread_pool.h"


static void thpool_thread_do(thpool_thread_parameter* tp_p);
static int thpool_jobqueue_init(thpool_t* tp_p);
static int thpool_jobqueue_removelast(thpool_t* tp_p);
static void thpool_jobqueue_add(thpool_t *tp_p, thpool_job_t *newjob_p);
static thpool_job_t* thpool_jobqueue_peek(thpool_t* tp_p);
static void thpool_jobqueue_empty(thpool_t* tp_p);

static int thpool_keepalive = 1;
/* create a mute and initialized */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* used to serialize queue access */

/**
 * thread pool initialize
 * thread_pool_numbers: the thread pool number(the number of concurrent threads)
 */
thpool_t *thpool_init(int thread_pool_numbers)
{
	int index = 0;
    thpool_t *thpool = NULL;

    if(!thread_pool_numbers || thread_pool_numbers < 1)
    {
        thread_pool_numbers = 1;
    }

    //allocate memory for thread pool
    thpool = (thpool_t*) malloc(sizeof(thpool_t));
    if(thpool == NULL)
    {
        printf("malloc thpool_t error");
        return NULL;
    }

    //distribution the thread number
    thpool->threadsN = thread_pool_numbers;
    thpool->threads = (pthread_t*) malloc(thread_pool_numbers * sizeof(pthread_t));
    if(thpool->threads == NULL)
    {
        printf("malloc thpool->threads error");
        return NULL;
    }

    // initialize job queue
    if(thpool_jobqueue_init(thpool))
    {
        return NULL;
    }

    thpool->jobqueue->queueSem = (sem_t*)malloc(sizeof(sem_t));
    sem_init(thpool->jobqueue->queueSem, 0, 0); // value start 0
    for(index = 0; index < thread_pool_numbers; index++)
    {
    	thpool_thread_parameter *thead_paramter = (thpool_thread_parameter*) malloc(sizeof(thpool_thread_parameter));;
    	thead_paramter->thpool = thpool;
    	thead_paramter->thread_index = index;
        pthread_create(&(thpool->threads[index]), NULL, (void *)thpool_thread_do, (void*)thead_paramter);
    }
    return thpool;
}

void thpool_destroy(thpool_t* tp_p)
{
    int i ;
    thpool_keepalive = 0;
    if (NULL == tp_p)
    {
    	return;
    }
    for(i = 0; i < (tp_p->threadsN); i++)
    {
        if(sem_post(tp_p->jobqueue->queueSem))
        {
            fprintf(stderr, "thpool_destroy(): Could not bypass sem_wait()\n");
        }
    }
    if(sem_post(tp_p->jobqueue->queueSem) != 0)
    {
        fprintf(stderr, "thpool_destroy(): Could not destroy semaphore\n");
    }
    for(i = 0;i < (tp_p->threadsN); i++)
    {
        pthread_join(tp_p->threads[i], NULL);
    }
    thpool_jobqueue_empty(tp_p);

    free(tp_p->threads);
    free(tp_p->jobqueue->queueSem);
    free(tp_p->jobqueue);
    free (tp_p);
}

/* Initialize queue */
static int thpool_jobqueue_init(thpool_t* tp_p)
{
    tp_p->jobqueue = (thpool_jobqueue*)malloc(sizeof(thpool_jobqueue));      /* MALLOC job queue */
    if (tp_p->jobqueue == NULL)
    {
    	return -1;
    }
    tp_p->jobqueue->tail = NULL;
    tp_p->jobqueue->head = NULL;
    tp_p->jobqueue->jobN = 0;
    return 0;
}

static void thpool_thread_do(thpool_thread_parameter *thread_paramter)
{
	thpool_t *tp_p = thread_paramter->thpool;
	int index = thread_paramter->thread_index;
	printf("index = %d\n", index);
    while(thpool_keepalive ==1)
    {
        if(sem_wait(tp_p->jobqueue->queueSem)) //thread block, until have data come
        {
            perror("thpool_thread_do(): Waiting for semaphore");
            exit(1);
        }
        if(thpool_keepalive)
        {
            FUNC function;
//            void *arg_buff;
            thpool_job_t *job_p;
            pthread_mutex_lock(&mutex);
            job_p = thpool_jobqueue_peek(tp_p);
            function = job_p->function;
//            arg_buff = job_p->arg;
            if(thpool_jobqueue_removelast(tp_p))
            {
               return ;
            }
            pthread_mutex_unlock(&mutex);
            function((void *)&job_p->arg, index); // our function
            free(job_p); // free the
        }
        else
        {
        	free(thread_paramter);
            return ;
        }
    }
    free(thread_paramter);
}

//get the queue tail
static thpool_job_t* thpool_jobqueue_peek(thpool_t* tp_p)
{
    return tp_p->jobqueue->tail;
}

/////ɾ����е����һ���ڵ�
static int thpool_jobqueue_removelast(thpool_t* tp_p)
{
	int reval = 0;
    thpool_job_t *theLastJob = NULL;
    if (tp_p == NULL)
    {
        return -1;
    }
    theLastJob = tp_p->jobqueue->tail;
    switch (tp_p->jobqueue->jobN)
    {
        case 0:
            return -1;
        case 1:
            tp_p->jobqueue->head = NULL;
            tp_p->jobqueue->tail = NULL;
            break;
        default:
            theLastJob->prev->next = NULL;
            tp_p->jobqueue->tail = theLastJob->prev;

    }
    (tp_p->jobqueue->jobN)--;
    sem_getvalue(tp_p->jobqueue->queueSem, &reval);
    return 0;
}

static void thpool_jobqueue_add(thpool_t *tp_p, thpool_job_t *newjob_p)
{
	int reval = 0;
    thpool_job_t *oldFirstJob = tp_p->jobqueue->head;

    newjob_p->next = NULL;
    newjob_p->prev = NULL;
    switch(tp_p->jobqueue->jobN)
    {
        case 0:
            tp_p->jobqueue->head = newjob_p;
            tp_p->jobqueue->tail = newjob_p;
            break;
        default:
            oldFirstJob->prev = newjob_p;
            newjob_p->next = oldFirstJob;
            tp_p->jobqueue->head = newjob_p;

    }
    (tp_p->jobqueue->jobN)++;
    sem_post(tp_p->jobqueue->queueSem);
    sem_getvalue(tp_p->jobqueue->queueSem, &reval);

    return;
}

// add to thread pool
int thpool_add_work(thpool_t* tp_p, void *(*function_p)(void* arg, int index), /*void *arg_p*/int socket_fd, char *recev_buffer)
{
    thpool_job_t *newjob  = (thpool_job_t*) malloc(sizeof(thpool_job_t));
    time_t now;

    time(&now);
    if(newjob == NULL)
    {
        fprintf(stderr, "thpool_add_work(): Could not allocate memory for new job\n");
        return -1;
    }
    newjob ->function = function_p;
    newjob->job_add_time = now;
//    newjob ->arg      = arg_p;
    memcpy(newjob->arg.recv_buffer, recev_buffer, BUFFER_SIZE);
    newjob->arg.fd = socket_fd;
    pthread_mutex_lock(&mutex);
    thpool_jobqueue_add(tp_p, newjob);
    pthread_mutex_unlock(&mutex);

    return 0;
}

// clean queue
static void thpool_jobqueue_empty(thpool_t* tp_p)
{
    thpool_job_t *curjob = tp_p->jobqueue->tail;

    while (tp_p->jobqueue->jobN)
    {
        tp_p->jobqueue->tail = curjob->prev;
        free (curjob);
        curjob = tp_p->jobqueue->tail;
        (tp_p->jobqueue->jobN)--;
    }
    tp_p->jobqueue->head = NULL;
    tp_p->jobqueue->tail = NULL;
}

int get_jobqueue_number(thpool_t* tp_p)
{
	if (NULL != tp_p && NULL != tp_p->jobqueue)
	{
		return tp_p->jobqueue->jobN;
	}
	else
	{
		return 0;
	}
}

int delete_timeout_job(thpool_t* tp_p, int time_out)
{
	int delete_number = 0;
	time_t now;
	thpool_job_t *curjob = tp_p->jobqueue->tail;

	time(&now);
	if (NULL != tp_p && NULL != tp_p->jobqueue)
	{
		pthread_mutex_lock(&mutex);
		while (tp_p->jobqueue->jobN)
		{
			if (NULL == curjob)
			{
				break;
			}
			if (now - curjob->job_add_time > time_out)
			{
				tp_p->jobqueue->tail = curjob->prev;
				free (curjob);
				curjob = tp_p->jobqueue->tail;
				(tp_p->jobqueue->jobN)--;
				delete_number++;
			}
			else
			{
				break;
			}
		}
		pthread_mutex_unlock(&mutex);
	}

	return delete_number;
}
