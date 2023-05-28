/*
 * 测试线程参数的传递
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

void *thmain(void *arg)
{
    printf("thmain ... \n");
    sleep(15);
    printf("thmain finish\n");
}

int main(int argc, char *argv[])
{
    pthread_t thid = 0;

    pthread_create(&thid,  NULL, thmain,  NULL);

    pthread_kill(thid, 2);

    printf("join ...\n");
    pthread_join(thid, NULL);
    printf("join finish\n");

    return 0;
}