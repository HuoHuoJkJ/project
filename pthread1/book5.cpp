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

void *thmain(void *arg);

int main(int argc, char *argv[])
{
    pthread_t thid = 0;

    struct st_args *stargs = new struct st_args;
    stargs->no = 10; strcpy(stargs->name, "测试线程");
    if (pthread_create(&thid, NULL, thmain, stargs) != 0) { printf("线程创建失败\n"); exit(-1); }

    printf("join ... \n");
    pthread_join(thid, NULL);
    printf("join ok  \n");

    return 0;
}

void *thmain(void *arg)
{
    struct st_args *pst = (struct st_args *)arg;
    printf("no = %d\nname = %s\n", pst->no, pst->name);
    printf("线程主函数\n");
    delete (int *)arg;
}