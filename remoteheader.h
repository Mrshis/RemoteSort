#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>


#define MAXQ 20
#define MAXLINE 4096
#define PORT 5000
#define ADDR "127.0.0.1"
#define LISTENQ 32
#define MAXEVENTS 5000

typedef struct worker
{
	void *(*process)(void *arg);
	void *arg;
	struct worker *next;
}thread_worker;

typedef struct
{
	pthread_mutex_t queue_lock;
	pthread_cond_t queue_ready;

	thread_worker *queue_header;
	pthread_t *threadid;
	int shutdown;
	int max_thread_num;
	int cur_queue_size;

}thread_pool;


void setnoblock(int);
void pool_init(int);
void *thread_routine(void *);
void pool_add_work(void* (*process)(void*), void *);
void destroy_pool();
void *deal_with_client(void *arg);
void quick_sort(int *, int, int);
int  *transfer(char *, int *, int, int*);
int  string_to_inter(char*, int);
void sig_int(int);
void inter_to_string(int, char*, int);
