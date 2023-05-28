/*
 * 在实际开发中，一般都是使用Linux中的线程库pthread
 * C11标准的线程没什么价值，功能也没有Linux线程库丰富
 * Linux线程库更接近底层
 * 本程序演示线程的创建
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

int var = 0;

void *thmain1(void *arg); // 线程主函数
void *thmain2(void *arg); // 线程主函数

int main(int argc, char *argv[])
{
    // int pthread_create(pthread_t *restrict thread,
    //                   const pthread_attr_t *restrict attr,
    //                   void *(*start_routine)(void *),
    //                   void *restrict arg);
    pthread_t thid1 = 0;
    pthread_t thid2 = 0;

    // 为了方便，我们将main函数创建的进程 命名为 主进程/主线程
    // 将pthread_create() 创建的线程   命名为 线程/子线程
    // 子线程运行的主函数               命名为 线程主函数

    if (pthread_create(&thid1, NULL, thmain1, NULL) != 0) { printf("线程创建失败\n"); exit(-1); }
    if (pthread_create(&thid2, NULL, thmain2, NULL) != 0) { printf("线程创建失败\n"); exit(-1); }

    printf("join ... \n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    printf("join ok  \n");

    return 0;
}

// 线程主函数
void *thmain1(void *arg)
{
    for (int ii = 0; ii < 5; ii ++)
    {
        sleep(ii);
        var ++;
        printf("thmain1 var = %d\n", var);
        if (ii == 1)
            pthread_exit((void *)1);
            // return 0;
    }
}

void *thmain2(void *arg)
{
    // exit(-1);
    for (int ii = 0; ii < 5; ii ++)
    {
        sleep(ii);
        printf("thmain2 var = %d\n", var);
        // int *cc = new int;
        // delete cc;
        // delete cc;
    }
}