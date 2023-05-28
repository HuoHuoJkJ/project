/*
 * 测试线程参数的传递
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

struct st_args
{
    int  no;
    char name[50];
};

void *thmain1(void *arg);
void *thmain2(void *arg);

int main(int argc, char *argv[])
{
    pthread_t thid1 = 0, thid2 = 0;

    if (pthread_create(&thid1, NULL, thmain1, NULL) != 0) { printf("线程创建失败\n"); exit(-1); }
    if (pthread_create(&thid2, NULL, thmain2, NULL) != 0) { printf("线程创建失败\n"); exit(-1); }

    sleep(10);

    printf("join ... \n");
    int *ret1, *ret2;
    pthread_join(thid1, (void **)(&ret1)); printf("ret1 = %d\n", ret1);
    pthread_join(thid2, (void **)(&ret2)); printf("ret2 = %d\n", ret2);
    printf("join ok  \n");

    return 0;
}

void *thmain1(void *arg)
{
    for (int ii = 0; ii < 3; ii ++)
    {
        sleep(1);
        printf("thmain1 sleep %d sec.\n", ii+1);
    }
    return (void *)1;
}

void *thmain2(void *arg)
{
    for (int ii = 0; ii < 5; ii ++)
    {
        sleep(1);
        printf("thmain2 sleep %d sec.\n", ii+1);
    }
    return (void *)2;
}