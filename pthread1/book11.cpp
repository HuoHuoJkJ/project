/*
 * 测试线程参数的传递
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

int  var = 0;

pthread_mutex_t mutex;  // 声明互斥锁

void *thmain(void *arg);

int main(int argc, char *argv[])
{
    pthread_mutex_init(&mutex, NULL);

    pthread_t thid1 = 0, thid2 = 0;

    pthread_create(&thid1,  NULL, thmain,  NULL);
    pthread_create(&thid2,  NULL, thmain,  NULL);

    printf("join ...\n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    printf("join finish\n");

    printf("var = %d\n", var);

    pthread_mutex_destroy(&mutex);
    return 0;
}

void *thmain(void *arg)
{
    for (int ii = 0; ii < 1000000; ii ++)
    {
        pthread_mutex_lock(&mutex);
        var ++;
        pthread_mutex_unlock(&mutex);
    }
}