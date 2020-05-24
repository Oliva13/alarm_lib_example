#ifndef __ALARMS_IMPL_H__
#define __ALARMS_IMPL_H__

typedef struct alarm_tag
{
    struct list_head    alist;
    //int                 seconds;
    time_t              time;   // seconds from EPOCH
    struct alarm_tag	*addr_curalarm;
    int 			   (*cbfalarm)(void *);
    void 			 	*cbfalarmarg;
} alarm_t;

struct head_alarm
{
	struct list_head   alist;
	pthread_mutex_t    alarm_mutex;
	pthread_cond_t     alarm_cond;
	pthread_t 	   	   thid;
	void		   	   (*cbthrexp)(void *);
    void 		        *cbthrexparg;
};

static alarm_t *current_alarm;
static int alarm_insert(alarm_t *alarm);
static int salarm_update(alarm_t *alarm);
static int salarm_delete(alarm_t *alarm);
static void add(struct list_head *old, struct list_head *new);
static void show_list(void);
static void dlist(struct list_head *head);

#endif
