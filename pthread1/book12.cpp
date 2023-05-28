/*
 * 测试线程参数的传递
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

int  var = 0;

void *thmain(void *arg);
void  handle(int sig);

int main(int argc, char *argv[])
{
    signal(15, handle);
    pthread_t thid1 = 0, thid2 = 0, thid3 = 0;

    pthread_create(&thid1,  NULL, thmain,  NULL);
    sleep(1);
    pthread_create(&thid2,  NULL, thmain,  NULL);
    sleep(1);
    pthread_create(&thid3,  NULL, thmain,  NULL);

    printf("join ...\n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);
    printf("join finish\n");

    printf("var = %d\n", var);

    pthread_rwlock_destroy(&rwlock);
    return 0;
}

void *thmain(void *arg)
{
    for (int ii = 0; ii < 100; ii ++)
    {
        printf("线程%lu开始申请读锁\n", pthread_self());
        pthread_rwlock_rdlock(&rwlock);
        printf("线程%lu申请读锁成功\n\n", pthread_self());
        sleep(5);
        pthread_rwlock_unlock(&rwlock);
        printf("线程%lu已经释放读锁\n\n", pthread_self());

        if (ii == 3) sleep(7);
    }
}

void  handle(int sig)
{
    printf("开始申请写锁\n");
    pthread_rwlock_wrlock(&rwlock);
    printf("申请写锁成功\n\n");
    sleep(5);
    pthread_rwlock_unlock(&rwlock);
    printf("已经释放写锁\n\n");
}