/*
 * 测试线程参数的传递
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

void *thmain1(void *arg);
void *thmain2(void *arg);

int main(int argc, char *argv[])
{
    pthread_t thid1 = 0, thid2 = 0;

    pthread_create(&thid1,  NULL, thmain1,  NULL);
sleep(1);
    pthread_create(&thid2,  NULL, thmain2,  NULL);

    pthread_join(thid1, NULL); pthread_join(thid2, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

void *thmain1(void *arg)
{
    printf("线程一开始申请互斥锁\n");
    pthread_mutex_lock(&mutex);
    printf("线程一申请互斥锁成功\n\n");
// sleep(3);
    printf("线程一开始等待条件变量\n");
    pthread_cond_wait(&cond, &mutex);
    printf("线程一等待条件变量成功\n\n");

    // pthread_mutex_unlock(&mutex);
}

void *thmain2(void *arg)
{

    printf("线程二开始申请互斥锁\n");
    pthread_mutex_lock(&mutex);
    printf("线程二申请互斥锁成功\n\n");

    printf("线程二已发送条件变量\n");
    pthread_cond_signal(&cond);

    sleep(5);

    printf("线程二已解锁互斥锁\n");
    pthread_mutex_unlock(&mutex);

    return 0;

    pthread_cond_signal(&cond); sleep(1);
    printf("线程二开始申请互斥锁\n");
    pthread_mutex_lock(&mutex);
    printf("线程二申请互斥锁成功\n\n");

    printf("线程二开始等待条件变量\n");
    pthread_cond_wait(&cond, &mutex);
    printf("线程二等待条件变量成功\n\n");
}