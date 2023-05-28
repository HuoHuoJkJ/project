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
void  thcleanup1(void *arg);
void  thcleanup2(void *arg);
void  thcleanup3(void *arg);

int main(int argc, char *argv[])
{
    pthread_t thid = 0;

    if (pthread_create(&thid, NULL, thmain, NULL) != 0) { printf("线程创建失败\n"); exit(-1); }

    printf("join ... \n");
    int *ret;
    int result = pthread_join(thid, (void **)(&ret));
    printf("result = %d   ret = %d\n", result, ret);
    printf("join ok  \n");

    return 0;
}

void *thmain(void *arg)
{
    pthread_cleanup_push(thcleanup1, NULL);  // 把线程清理函数入栈
    pthread_cleanup_push(thcleanup2, NULL);  // 把线程清理函数入栈
    pthread_cleanup_push(thcleanup3, NULL);  // 把线程清理函数入栈

    for (int ii = 0; ii < 3; ii ++)
    {
        sleep(1);
        printf("thmain sleep %d sec.\n", ii+1);
    }

    return 0;

    pthread_cleanup_pop(0);                 // 把线程清理函数出栈
    pthread_cleanup_pop(0);                 // 把线程清理函数出栈
    pthread_cleanup_pop(0);                 // 把线程清理函数出栈

    return (void *)1;
}

void  thcleanup1(void *arg)
{
    // 在这里释放 关闭文件、断开网络连接、回滚数据库事务、释放锁等
    printf("clean1 ok\n");
}

void  thcleanup2(void *arg)
{
    // 在这里释放 关闭文件、断开网络连接、回滚数据库事务、释放锁等
    printf("clean2 ok\n");
}

void  thcleanup3(void *arg)
{
    // 在这里释放 关闭文件、断开网络连接、回滚数据库事务、释放锁等
    printf("clean3 ok\n");
}