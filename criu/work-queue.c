#include "work-queue.h"
#include <pthread.h>
#include <stdlib.h>

static struct work_item
{
    int (*work)(void *);
    void *args;
    int fd;
    int nstype;
};

static struct work_queue
{
    struct work_item **works;
    pthread_mutex_t m;
    pthread_cond_t cond;
    pthread_t thread;
    int capacity;
    int front;
    int rear;
    int stop;
};

static struct work_queue q;

static int empty()
{
    return q.front == q.rear;
}

static int full()
{
    return ((q.rear + 1) % q.capacity) == q.front;
}

static int push(struct work_item *item)
{
    pthread_mutex_lock(&q.m);
    if (full())
    {
        pthread_mutex_unlock(&q.m);
        return WORK_QUEUE_FULL;
    }
    q.works[q.rear] = item;
    q.rear = (q.rear + 1) % q.capacity;
    pthread_mutex_unlock(&q.m);
    pthread_cond_signal(&q.cond);
    return 0;
}


static void *consume_work(void *t)
{
    struct work_item *item;
    while (1)
    {
        pthread_mutex_lock(&q.m);
        while (empty())
        {
            pthread_cond_wait(&q.cond, &q.m);
            if (q.stop)
                goto out;
        }
        item = q.works[q.front];
        q.front = (q.front + 1) % q.capacity;
        pthread_mutex_unlock(&q.m);
        item->work(item->args);
        free(item);
    }
out:
    pthread_exit(NULL);
}

int add_work(int (*work)(void *), void *args, int fd, int nstype)
{
    struct work_item *item;
    item = (struct work_item *)malloc(sizeof(struct work_item));
    item->work = work;
    item->args = args;
    item->fd = fd;
    item->nstype = nstype;
    return push(item);
}

void stop_work_queue()
{
    pthread_mutex_lock(&q.m);
    q.stop = 1;
    pthread_mutex_unlock(&q.m);
    pthread_cond_signal(&q.cond);
    pthread_join(q.thread, NULL);
}

void start_work_queue()
{
    pthread_create(&q.thread, NULL, consume_work, 0);
}

int init_work_queue(int capacity)
{
    if (capacity <= 0)
        return WORK_QUEUE_INIT_FAIL;
    q.works = (struct work_item **)malloc((capacity + 1) * sizeof(struct work_item *));
    pthread_mutex_init(&q.m, NULL);
    pthread_cond_init(&q.cond, NULL);
    q.capacity = capacity;
    q.front = q.rear = 0;
    q.stop = 0;
    return 0;
}