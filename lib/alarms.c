#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <limits.h>
#include "alarms.h"
#include "alarms_impl.h"

static struct head_alarm halarm;

void *alarm_thread(void *arg)
{  
	alarm_t *alarm;
	struct timespec cond_time, timespec;
	int status = 0, expired;
	syslog(LOG_INFO, "start ok...");
	while (1) 
	{
		status = pthread_mutex_lock(&halarm.alarm_mutex);
		if(status != 0)
		{
			syslog(LOG_ERR,"Error: pthread_mutex_lock");
			halarm.cbthrexp(halarm.cbthrexparg);
			return (void *)status;
		}	
		current_alarm->time = 0;
		while (list_empty(&halarm.alist)) 
		{
		        status = pthread_cond_wait(&halarm.alarm_cond, &halarm.alarm_mutex);
    			if (status != 0)
			{
				syslog(LOG_ERR,"Error: Wait on cond");
				pthread_mutex_unlock(&halarm.alarm_mutex);
				halarm.cbthrexp(halarm.cbthrexparg);
				return (void *)status;
			}	 
        }
		alarm = (alarm_t *)halarm.alist.prev;
		if(clock_gettime(CLOCK_MONOTONIC, &timespec) != 0)
        {
			syslog(LOG_ERR,"Error: clock_gettime");
			pthread_mutex_unlock(&halarm.alarm_mutex);
			halarm.cbthrexp(halarm.cbthrexparg);
			return (void *)status;
		}
		expired = 0;
		if (alarm->time > timespec.tv_sec)
		{
	        cond_time.tv_sec = alarm->time;
    		cond_time.tv_nsec = 0;
			current_alarm->time = alarm->time;
			current_alarm->addr_curalarm = alarm;
			while(1) 
			{
				if(current_alarm->time != alarm->time)
				{
					break;
				}
				status = pthread_cond_timedwait(&halarm.alarm_cond, &halarm.alarm_mutex, &cond_time);
				if (status == ETIMEDOUT) 
				{
					expired = 1;
					break;
				}
				if (status != 0)
				{
					syslog(LOG_ERR,"Error: Cond timedwait\n");
					pthread_mutex_unlock(&halarm.alarm_mutex);
					halarm.cbthrexp(halarm.cbthrexparg);
					return (void *)status;
				}	
			}
		}
		else
		{
			expired = 1;
		}	
	    if (expired) 
	    {		
			if(alarm->cbfalarm != NULL)
			{
				alarm->time = 0;
				if(!list_del_repeated(&alarm->alist))
				{
					list_del(&alarm->alist);
				}	
				pthread_mutex_unlock(&halarm.alarm_mutex);
				alarm->cbfalarm(alarm->cbfalarmarg);
			}	
			else
			{
				syslog(LOG_ERR,"Error: pthread: alarm->cbfalarm\n");
			}
		}
		pthread_mutex_unlock(&halarm.alarm_mutex);
  }
  return (void *)status;
}

int alarms_init(void (*funcexp)(void *), void *arg)
{
    int status = 0;
    pthread_condattr_t condattr;
	INIT_LIST_HEAD(&halarm.alist);
	current_alarm  = malloc(sizeof(alarm_t));
	if (current_alarm == NULL) 
	{
		syslog(LOG_ERR, "Error: current_alarm malloc %d (%s)\n", errno, strerror(errno));
		return 1;
	}
	if (funcexp == NULL) 
	{
		syslog(LOG_ERR, "Error: alarms_init funcexp is NULL\n");
		free(current_alarm);
		return 1;
	}
	
	halarm.cbthrexp = funcexp;
	halarm.cbthrexparg = arg;

	status = pthread_condattr_init(&condattr);
	if(status)
	{
       syslog(LOG_ERR,"Error: pthread_condattr_init");
	   free(current_alarm);
	   return 1;
	}	
	
	status = pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
	if(status)
	{
       syslog(LOG_ERR,"Error: pthread_condattr_setclock - %d, %d (%s)", status, errno, strerror(errno));
	   free(current_alarm);
	   return 1;
	}	

	status = pthread_mutex_init(&halarm.alarm_mutex, NULL);
	if(status)
	{
       syslog(LOG_ERR,"Error: pthread_mutex_init");
	   free(current_alarm);
	   return 1;
	}	
	
	status = pthread_cond_init(&halarm.alarm_cond, &condattr);
	if(status)
	{
       syslog(LOG_ERR,"Error: pthread_cond_init");
	   free(current_alarm);
	   pthread_mutex_destroy(&halarm.alarm_mutex);
	   return 1;
	}	

	status = pthread_condattr_destroy(&condattr);
	if(status)
	{
	   syslog(LOG_ERR,"Error: pthread_condattr_destroy - %d, %d (%s)", status, errno, strerror(errno));
	   free(current_alarm);
	   return 1;
	}	

	status = pthread_create(&halarm.thid, NULL, alarm_thread, NULL);
	if (status != 0)
	{
		syslog(LOG_ERR, "Create alarm thread %d (%s)", errno, strerror(errno));
		free(current_alarm);
		pthread_cond_destroy(&halarm.alarm_cond);
		pthread_mutex_destroy(&halarm.alarm_mutex);
		return 1;
	}
	return 0;
}

void* alarm_add(int sec, int (*cbfalarm)(void *), void *arg)
{
   alarm_t *alarm;
   struct timespec timespec;
   int status = 0;
   if(cbfalarm == NULL)
   {
	  syslog(LOG_ERR,"Error: cbfalarm is NULL");
	  return NULL;
   }
   // Создаем new alarm
   alarm = (alarm_t *)malloc(sizeof(alarm_t));
   if(alarm == NULL)
   {
	  syslog(LOG_ERR,"Error malloc: alarm == NULL\n");
	  return NULL;
   }
   status = pthread_mutex_lock(&halarm.alarm_mutex);
   if(status != 0)
   {
	  syslog(LOG_ERR,"Error: alarm_delete pthread_mutex_lock");
	  free(alarm);
	  return NULL;
   }	

   if (clock_gettime(CLOCK_MONOTONIC, &timespec) != 0)
   {
		syslog(LOG_ERR,"Error: clock_gettime");
	    free(alarm);
		pthread_mutex_unlock(&halarm.alarm_mutex);
		return NULL;
   }
   
   alarm->time = timespec.tv_sec + sec;
   alarm->cbfalarm = cbfalarm;
   alarm->cbfalarmarg = arg;
   status = alarm_insert(alarm);
   if(status != 0)
   {
	  syslog(LOG_ERR,"Error: alarm_insert");
	  free(alarm);
	  pthread_mutex_unlock(&halarm.alarm_mutex);
	  return NULL;
   }	
   pthread_mutex_unlock(&halarm.alarm_mutex);
   return alarm;
}

int alarm_update(void *alarm, int sec)
{	
	int status = 0;
	alarm_t *palarm = alarm;
	struct timespec timespec;
	status = pthread_mutex_lock(&halarm.alarm_mutex); 
	if(status != 0)
	{
		syslog(LOG_ERR,"pthread_mutex_lock\n");
		return 1;
	}
	if(palarm == NULL)
	{
		syslog(LOG_ERR,"Error malloc: alarm == NULL\n");
		pthread_mutex_unlock(&halarm.alarm_mutex); 
		return 1;
	}

        if(clock_gettime(CLOCK_MONOTONIC, &timespec) != 0)
	{
		syslog(LOG_ERR,"clock_gettime\n");
		pthread_mutex_unlock(&halarm.alarm_mutex); 
		return 1;
    }
    palarm->time = timespec.tv_sec + sec;
	status = salarm_update(palarm);
	if(status != 0)
	{	
		syslog(LOG_ERR,"update_new_alarm\n");
		pthread_mutex_unlock(&halarm.alarm_mutex); 
		return 1;
	}		
	pthread_mutex_unlock(&halarm.alarm_mutex); 
	return 0;
}

int alarm_delete(void *alarm)
{
	int status;
	alarm_t *palarm = (alarm_t *)alarm;
	status = pthread_mutex_lock(&halarm.alarm_mutex);
	if(status != 0)
	{
		syslog(LOG_ERR,"Error: alarm_delete pthread_mutex_lock");
		return 1;
	}	
	if(palarm == NULL)
	{
		syslog(LOG_ERR,"Error malloc: alarm == NULL\n");
		pthread_mutex_unlock(&halarm.alarm_mutex);
		return 1;
	}
	palarm->time = 0;
	status = salarm_delete(palarm);
	if (status != 0)
	{
		syslog(LOG_ERR,"Error: alarm_delete\n");
		pthread_mutex_unlock(&halarm.alarm_mutex); 
		return 1;
	}
	pthread_mutex_unlock(&halarm.alarm_mutex); 
	return 0;
}

int alarms_destroy(void)
{
	int status = 0;
	void *res;
	struct list_head *pos, *q;
	alarm_t  *tmp;
	status = pthread_mutex_lock(&halarm.alarm_mutex); 
	if(status != 0)
	{
		syslog(LOG_ERR,"pthread_mutex_lock\n");
		return 1;
	}
	dlist(&halarm.alist);
	pthread_mutex_unlock(&halarm.alarm_mutex);
	status = pthread_cancel(halarm.thid);
	if (status != 0)
    {
		 syslog(LOG_ERR,"pthread_cancel");
		 return 1;
	}
	status = pthread_join(halarm.thid, &res);
    if(status != 0)
    {
		syslog(LOG_ERR,"pthread_join");
		return 1;
	}
    if (res == PTHREAD_CANCELED)
    {
        syslog(LOG_ERR,"thread was canceled\n");
    } 
    else
    {   
        syslog(LOG_ERR,"thread wasn't canceled (shouldn't happen!)\n");
    }
    status = pthread_mutex_destroy(&halarm.alarm_mutex);
    if(status != 0)
    {
		syslog(LOG_ERR,"pthread_join");
		return 1;
    }
    status = pthread_cond_destroy(&halarm.alarm_cond);
    if(status != 0)
    {
		syslog(LOG_ERR,"pthread_join");
		return 1;
    }
    return 0;
}

static void dlist(struct list_head *head)
{
	struct list_head *ptr;
	ptr = halarm.alist.prev;
	alarm_t *aprev;
	while(ptr != &halarm.alist)
	{
	    aprev = list_entry(ptr, alarm_t, alist);
	    if(aprev != NULL)
	    {
			syslog(LOG_ERR,"free aprev %p\n", aprev);
	    	free(aprev);
	    }
	    ptr = ptr->prev;
	}
}

static void show_list(void)
{
	struct list_head *ptr;
	ptr = halarm.alist.prev;
	alarm_t *aprev;
	while(ptr != &halarm.alist)
	{
	    aprev = list_entry(ptr, alarm_t, alist);
	    if(aprev != NULL)
	    {
		syslog(LOG_ERR,"alarm - %p\n", aprev);
	    }
	    ptr = ptr->prev;
	}
}

static void add(struct list_head *old, struct list_head *new)
{
	new->next = old->next;
	new->prev = old;
	old->next->prev = new;
	old->next = new;
}

/* mutex held */
static int alarm_insert(alarm_t *alarm)
{
	int status = 0;
	alarm_t *aprev;
	alarm_t *old, *node;
	struct list_head *ptr;
	ptr = halarm.alist.prev;
	while(ptr != &halarm.alist)
	{
	    aprev = list_entry(ptr, alarm_t, alist);
	    if(aprev->time >= alarm->time)
	    {
	    	add(ptr, &alarm->alist);
			break;
	    }	
	    ptr = ptr->prev;
	}
	if(ptr == &halarm.alist)
	{
		add(&halarm.alist, &alarm->alist);
	}	
	if (current_alarm->time == 0 || alarm->time < current_alarm->time)
	{
       current_alarm->time = alarm->time;
	   current_alarm->addr_curalarm = alarm;
	   status = pthread_cond_signal(&halarm.alarm_cond);
       if(status != 0)
	   {
          syslog(LOG_ERR, "Error: Signal cond");
	      return 1;
	   }
	}
	return 0;
}

static int salarm_update(alarm_t *alarm)
{
	int status;
	status = salarm_delete(alarm);
	if(status)
	{
		return 1;
	}
	status = alarm_insert(alarm);
	if(status)
		return 1;
	return 0;
}

static int salarm_delete(alarm_t *alarm)
{
	int status = 0;
	if(list_del_repeated(&alarm->alist))
	{
		return 1;
	}	
	list_del(&alarm->alist);
	if(alarm == (alarm_t *)current_alarm->addr_curalarm)
	{
		status = pthread_cond_signal(&halarm.alarm_cond);
		if (status != 0)
		{
			syslog(LOG_ERR,"Error: Signal cond");
			return 1;
		}
	}
	return 0;
}

