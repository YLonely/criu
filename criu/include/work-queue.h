#ifndef __CR_WORKQUEUE_H__
#define __CR_WORKQUEUE_H__

#define WORK_QUEUE_FULL -1
#define WORK_QUEUE_EMPTY -2
#define WORK_QUEUE_INIT_FAIL -3

extern int add_work(int (*)(void *), void *, int, int);
extern int init_work_queue(int capacity);
extern void start_work_queue();
extern void stop_work_queue();

#endif