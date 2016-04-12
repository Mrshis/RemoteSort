/*************************************************************************
	> File Name: remotesort.c
	> Author: shi
	> Mail: 1249224617@qq.com 
	> Created Time: Sat 12 Mar 2016 05:21:18 PM CST
 ************************************************************************/

#include "remoteheader.h"


thread_pool *pool;

int
main(int argc, char** argv)
{
	struct epoll_event ev, events[MAXQ];
	int epfd, nfds, listenfd, i, confd, tfd;
	socklen_t clilen;
	struct sockaddr_in servaddr, cliaddr;
	char buff[MAXLINE];
	int e;

	listenfd=socket(AF_INET, SOCK_STREAM, 0);
	setnoblock(listenfd);// set the fd noblock
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(PORT);
	inet_pton(AF_INET, ADDR, &servaddr.sin_addr);
	e=bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if( e==-1)
	{
		perror("bind error");
		return;
	}
	listen(listenfd, LISTENQ);

	epfd=epoll_create(5000);
	ev.data.fd=listenfd;
	ev.events=EPOLLIN | EPOLLET;
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

	ev.data.fd=SIGINT;
	ev.events=EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, SIGINT, &ev);


	pool_init(8);

	for( ;;)
	{
		nfds=epoll_wait(epfd, events, MAXQ, -1);
		for( i=0; i<nfds; i++)
		{
			tfd=events[i].data.fd;
			if( tfd==listenfd)
			{
				//  accept new connection
				while( (confd=accept(tfd, NULL/*(struct sockaddr*)&cliaddr*/, NULL/*&clilen*/))>0)
				{
					setnoblock(confd);
					ev.data.fd=confd;
					ev.events=EPOLLIN | EPOLLET;
					epoll_ctl(epfd, EPOLL_CTL_ADD, confd, &ev);
					
				}
				if( confd==-1)
				{
					if( confd!=EAGAIN && errno!=ECONNABORTED && errno!=EPROTO && errno!=EINTR)
						perror("accept error");
				}
				continue;// if the fd==listenfd , and there is no need to check next

			}
			else if( tfd==SIGINT)
			{
				perror("Received signal SIGIN");
				destroy_pool();
				return;
			}
			else if( events[i].events & EPOLLIN)
			{
				int t[2];
				t[0]=tfd;
				t[1]=epfd;
				pool_add_work(deal_with_client, t);
			}
			
		}
	}
	destroy_pool();
}

void
setnoblock(int fd)
{
	int opt;
	opt=fcntl(fd, F_GETFL);
	opt=opt | O_NONBLOCK;
	fcntl(fd, F_SETFL, opt);
}

void *
deal_with_client(void *arg)
{
	char buf[1024]={0};
	int data[1024], l, i;
	int fd=*(int*)arg;
	int epfd=*((int*)arg+1);
	struct epoll_event ev;
	ev.data.fd=fd;
	ev.events=EPOLLIN;

	l=read(fd, buf, sizeof(buf));


	transfer(buf, data, l, &l);
	quick_sort(data, 0, l);
	bzero(buf, sizeof(buf));
	for( i=0; i<=l; i++)
	{
		char tmp[8];
		bzero(tmp, 8);
		inter_to_string(data[i], tmp, 8);
		strcat(buf, tmp);
		strcat(buf, ",");
	}
	write(fd, buf, strlen(buf)-1);

	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);

	close(fd);
	return;

}
int *
transfer(char *c, int *s, int len, int *l)
{
	char temp[8];
	int i=0;
	int j=0, k=0;
	for( i=0; i<len;i ++)
	{
		if( c[i]==',')
		{
			s[k++]=atoi(temp);
			j=0;
			bzero(temp, 8);
			continue;
		}
		temp[j++]=c[i];		
	}
	s[k]=atoi(temp);
	*l=k;
	return s;
}
void
quick_sort(int *s, int left, int right)
{
	if( left<right)
	{
		int low=left;
		int high=right;
		int key=s[left];

		while( low<high)
		{
			while( low<high && s[high]>=key)
				high--;
			s[low]=s[high];
			while( low<high && s[low]<=key)
				low++;
			s[high]=s[low];
		}
		s[low]=key;
		quick_sort(s, left, low-1);
		quick_sort(s, low+1, right);
	}
}

void
inter_to_string(int num, char *s, int len)
{
	sprintf(s, "%d", num);
	return;
}
