/*
 * 测试线程参数的传递
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

void *thmain(void *arg);

int main(int argc, char *argv[])
{
    pthread_t thid = 0;

    pthread_attr_t attr;        // 声明线程属性的数据结构
    pthread_attr_init(&attr);   // 初始化数据结构
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);    // 设置线程的属性

    if (pthread_create(&thid, NULL, thmain, NULL) != 0) { printf("线程创建失败\n"); exit(-1); }
    pthread_attr_destroy(&attr);// 销毁数据结构

    sleep(5);

    // pthread_detach(thid);

    printf("join ... \n");
    int *ret;
    int result = pthread_join(thid, (void **)(&ret));
    printf("result = %d   ret = %d\n", result, ret);
    printf("join ok  \n");

    return 0;
}

void *thmain(void *arg)
{
    // pthread_detach(pthread_self());

    for (int ii = 0; ii < 3; ii ++)
    {
        sleep(1);
        printf("thmain sleep %d sec.\n", ii+1);
    }
    return (void *)1;
}