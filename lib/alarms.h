#ifndef __ALARMS_H__
#define __ALARMS_H__

#include "list.h"

int alarms_init(void (*cbthrexp)(void *), void *arg);
void* alarm_add(int sec, int (*cbfalarm)(void *), void *arg);
int alarm_update(void *alarm, int sec);
int alarm_delete(void *alarm);
int alarms_destroy(void);

#endif
