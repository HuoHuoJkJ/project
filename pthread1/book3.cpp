/*
 * 测试线程参数的传递
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

int var = 0;

void *thmain1(void *arg);
void *thmain2(void *arg);
void *thmain3(void *arg);
void *thmain4(void *arg);
void *thmain5(void *arg);

int val = 0;

int main(int argc, char *argv[])
{
    // long ii = 10;
    // int oo = ii;
    // printf("%ld\n", oo);

    // return 0;

    // int ii = 10;
    // void *pv;
    // pv = (void *)(long)ii;
    // printf("pv = %p\n", pv);

    // int oo;
    // oo = (int)(long)pv;
    // printf("oo = %d\n", oo);

    // return 0;

    pthread_t thid1 = 0, thid2 = 0, thid3 = 0, thid4 = 0, thid5 = 0;

    val = 1;
    if (pthread_create(&thid1, NULL, thmain1, (void *)(long)val) != 0) { printf("线程创建失败\n"); exit(-1); }
    val = 2;
    if (pthread_create(&thid2, NULL, thmain2, (void *)(long)val) != 0) { printf("线程创建失败\n"); exit(-1); }
    val = 3;
    if (pthread_create(&thid3, NULL, thmain3, (void *)(long)val) != 0) { printf("线程创建失败\n"); exit(-1); }
    val = 4;
    if (pthread_create(&thid4, NULL, thmain4, (void *)(long)val) != 0) { printf("线程创建失败\n"); exit(-1); }
    val = 5;
    if (pthread_create(&thid5, NULL, thmain5, (void *)(long)val) != 0) { printf("线程创建失败\n"); exit(-1); }

    printf("join ... \n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);
    pthread_join(thid4, NULL);
    pthread_join(thid5, NULL);
    printf("join ok  \n");

    return 0;
}

void *thmain1(void *arg)
{
    printf("线程1主函数：val = %d\n", (int)(long)arg);
}

void *thmain2(void *arg)
{
    printf("线程2主函数：val = %d\n", (int)(long)arg);
}

void *thmain3(void *arg)
{
    printf("线程3主函数：val = %d\n", (int)(long)arg);
}

void *thmain4(void *arg)
{
    printf("线程4主函数：val = %d\n", (int)(long)arg);
}

void *thmain5(void *arg)
{
    printf("线程5主函数：val = %d\n", (int)(long)arg);
}