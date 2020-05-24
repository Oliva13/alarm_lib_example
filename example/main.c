#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <syslog.h>
#include "alarms.h"

int alarmfunc(void *arg)
{
    static int cnt = 0;
    int len = 0; 
    char *str;

    len = strlen((char *)arg);
    str = malloc(len * sizeof(char));
    strncpy(str, (char *)arg, len+1);

    syslog(LOG_ERR,"<<<<<<<<<<<<<<<< alarm callback func >>>>>>>>>>>>>>>>>>>\n");
    syslog(LOG_ERR,"arg - %p", arg);
    syslog(LOG_ERR,"len - %d", len);
    syslog(LOG_ERR,"str - %s", str);
    if(cnt == 1)
    {
        //alarms_destroy();
        exit(0);
    }
    cnt++;
    return 0;
}

void err_thread(void *arg)
{
	exit(1);
}

int main (int argc, char *argv[])
{
    void *alarm, *alarm1, *alarm2;

    struct list_head *pos, *q;

    int sec = 0;

    sec = 5;

    alarms_init(err_thread, NULL);
    syslog(LOG_ERR,"alarm_add\n");

    alarm = alarm_add(sec, alarmfunc, "Hello");
    sec = 10;
    alarm1 = alarm_add(sec, alarmfunc, "Oppa");
//    sec = 15;
//    alarm2 = alarm_add(sec, alarmfunc, "Blabla");

//    syslog(LOG_ERR,"alarm - %p\n", alarm);
//    syslog(LOG_ERR,"alarm - %p\n", alarm1);
//    syslog(LOG_ERR,"alarm - %p\n", alarm2);

//    show_list();
//  alarm_delete(alarm1);
//  alarm_delete(alarm2);
//  sec = 17;
//  alarm_update(alarm, sec);
//    sleep(10);
//    destroy_alarm();
    for(;;);
    return 0;
}
