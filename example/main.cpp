#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "list.h"
#include "hash.h"
#include "alarms.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_map>
#include <sys/time.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <cstring>
#include <string>
#include <functional>
#include <cassert>

using namespace std;

static pthread_t mpd_thr_create, mpd_thr_del;

struct thread_args *dash_thr;


#define id_cliend_last(head) list_entry(list_last(head), mpd_t, link)
#define id_cliend_first(head) list_entry(list_first(head), mpd_t, link)

typedef struct id_client
{
	string ip_addr;
	unsigned short port;

}id_client_t;

typedef struct
{
	char *path_file;
	unsigned long timestamp;
	struct list_head link;
}mpd_t;

struct thread_args
{
    pthread_cond_t	  thread_flag_cv;
    pthread_mutex_t	  thread_flag_mutex;
    void			  *data_pthread;
    int			 	  thread_flag;
    //mpd_t      	  *mpd_file;
};

struct list_head gl_mpd_link;

static unsigned long get_uptime(void)
{
	struct timespec tm;
	if (clock_gettime(CLOCK_MONOTONIC, &tm) != 0)
		return 0;
	return tm.tv_sec;
}

void init_start(struct thread_args *ptr)
{
	pthread_mutex_init(&ptr->thread_flag_mutex, NULL);
	pthread_cond_init(&ptr->thread_flag_cv, NULL);
	ptr->thread_flag = 0;
}

void init_close(struct thread_args *ptr)
{
	ptr->thread_flag = 1;
	pthread_mutex_destroy(&ptr->thread_flag_mutex);
	pthread_cond_destroy(&ptr->thread_flag_cv);
}

static void *create_mpd(void *arg)
{
	int res = 0;
	int count = 0;
	struct thread_args *p = (struct thread_args *)arg;
	char *fname = strndup("./record", 128);
	FILE *in;
	while(1)
	{
		count++;
		printf("create_mpd...\n");
		sprintf(fname,"./record%d", count);
		mpd_t *pmpd =(mpd_t *)malloc(sizeof(mpd_t));
		pmpd->path_file = strdup(fname);
		pmpd->timestamp = get_uptime();
		//create_file;
		in = fopen(fname, "w");
		printf("create: path_file - %s, timestamp - %d\n", pmpd->path_file, pmpd->timestamp);
		list_insert_after(&pmpd->link, &gl_mpd_link);
		sleep(1);
		pthread_cond_signal(&p->thread_flag_cv);
	}
	free(fname);
	
}

struct EqualCp
{
    bool operator()(const id_client_t & l, const id_client_t & r) const
    {
		return l.ip_addr == r.ip_addr && l.port == r.port;
    }
};

struct HashCp
{
    size_t operator()(const id_client_t & id) const
    {		
		return std::hash<string>()(id.ip_addr) ^ std::hash<unsigned short>()(id.port);
    }
};

unordered_map<id_client_t, mpd_t*, HashCp, EqualCp> clients;

static void *del_mpd(void *arg)
{
	int res = 0;
	struct thread_args *p = (struct thread_args *)arg;
	mpd_t *prt_mpd;
	int first_step = 0;	
	unsigned long prev_timestamp = 0;
	
	while(1)
	{
		//del_file;
		printf("dele_mpd...\n");
		for (auto& e : clients) 
		{
			printf("%p\n", e.second);
			prt_mpd = e.second;
		}
		printf("delete: prt_mpd->path_file - %s, timestamp - %d\n", prt_mpd->path_file, prev_timestamp);
		unlink(prt_mpd->path_file);
		sleep(3);
	}	
}

int http_callback(void *arg)
{
    printf("callback....\n");
    fflush(stdout);
    if(clients.empty())
    {
		printf("clients is empty\n");
		struct thread_args *dash_thr = (struct thread_args *)malloc(sizeof(struct thread_args));
		init_start(dash_thr);
		pthread_create(&mpd_thr_create, NULL, create_mpd, dash_thr);
		//start thread1 for mpd
		//ждем создание файла cv
		printf("wait...\n");
		pthread_cond_wait(&dash_thr->thread_flag_cv, &dash_thr->thread_flag_mutex);
		printf("run...\n");
		clients[{"192.168.11.145", 5555}] = id_cliend_last(&gl_mpd_link);
		//thread3 удаляет
		pthread_create(&mpd_thr_del, NULL,  del_mpd, dash_thr);
    }
    else
    {
		printf("clients is not empty\n");
		auto it = clients.find({"192.168.11.145", 555});
		if (it  == clients.end())
		{
			printf("взять последний tail...\n");
			clients[{"192.168.11.145", 5555}] = id_cliend_last(&gl_mpd_link);
			printf("отправляем ответ на сервак1...\n");
		}
		else
		{
			printf("взять next...\n");
			assert(it->second->link.next);
			clients[{"192.168.11.145", 555}] = id_cliend_first(&it->second->link);
			printf("отправляем ответ на сервак2...\n");
		}
		//sleep(4);
	}
	pthread_join(mpd_thr_create, NULL);
	pthread_join(mpd_thr_del, NULL);
}

void err_thread(void *arg)
{
	exit(1);
}

int main()
{
	int err = 0;
	void *alarm;
	int sec = 0;
	id_client_t idclient;
	LIST_INIT_HEAD(&gl_mpd_link);
	init_close(dash_thr);
	alarms_init(err_thread, NULL);
        printf("alarm_add\n");
        sec = 5;
        alarm = alarm_add(sec, http_callback, &idclient);
	free(dash_thr);
	for(;;);
	return 0;
}














