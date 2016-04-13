/*************************************************************************
	> File Name: thread_pool.c
	> Author: shi
	> Mail: 1249224617@qq.com 
	> Created Time: Sun 13 Mar 2016 10:34:23 AM CST
 ************************************************************************/

#include "remoteheader.h"

extern thread_pool *pool;


void
pool_init(int max_thread_num)
{
	pool=(thread_pool*)malloc(sizeof(thread_pool));
	pthread_mutex_init(&(pool->queue_lock), NULL);
	pthread_cond_init(&(pool->queue_ready), NULL);
	pool->queue_header=NULL;
	pool->threadid=(pthread_t*)malloc(max_thread_num * sizeof(pthread_t));
	pool->shutdown=0;
	pool->max_thread_num=max_thread_num;
	pool->cur_queue_size=0;

	int i;
	for( i=0; i<max_thread_num; i++)
		pthread_create(&(pool->threadid[i]), NULL, thread_routine, NULL);
}
void *
thread_routine(void *arg)
{
	while( 1)
	{
		pthread_mutex_lock(&(pool->queue_lock));
		while( pool->cur_queue_size==0 && !pool->shutdown)// pthread_cond_signal() may wake up more than one thread
			pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));

		if( pool->shutdown)
		{
			pthread_mutex_unlock(&(pool->queue_lock));
			printf("thread %d exit\n", pthread_self());
			pthread_exit(NULL);
		}

		pool->cur_queue_size--;

		thread_worker *worker=pool->queue_header;
		pool->queue_header=worker->next;
		pthread_mutex_unlock(&(pool->queue_lock));

		(*(worker->process))(worker->arg);
		free(worker);
		worker=NULL;
	}
	pthread_exit(0);
}
void
pool_add_work(void *(*process)(void *arg), void *arg)
{
	thread_worker *newworker=(thread_worker*)malloc(sizeof(thread_worker));
	newworker->process=process;
	newworker->arg=arg;
	newworker->next=NULL;

	pthread_mutex_lock(&(pool->queue_lock));
	thread_worker *member=pool->queue_header;
	if( member!=NULL)
	{
		while(member->next!=NULL)
			member=member->next;
		member->next=newworker;
	}
	else
		pool->queue_header=newworker;

	pool->cur_queue_size++;
	pthread_mutex_unlock(&(pool->queue_lock));

	pthread_cond_signal(&(pool->queue_ready));
	return;

}
void
destroy_pool()
{
	if( pool->shutdown)
		return;
	pool->shutdown=1;
	pthread_cond_broadcast(&(pool->queue_ready));
	
	int i;
	for( i=0; i<pool->max_thread_num; i++)
		pthread_join(pool->threadid[i], NULL);
	free(pool->threadid);
	thread_worker *head=NULL;
	while( pool->queue_header!=NULL)
	{
		head=pool->queue_header;
		pool->queue_header=pool->queue_header->next;
		free(head);
	}
	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->queue_ready));
	free(pool);
	pool=NULL;

	return;
}
