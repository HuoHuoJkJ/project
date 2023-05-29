/*
 * 测试线程参数的传递
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <vector>

using namespace std;

struct st_args
{
    int  mesgid;
    char message[1024];
};

vector<struct st_args> vcache;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

void  incache(int sig);         // 生产者函数
void *outcache(void *arg);      // 消费者函数

int main(int argc, char *argv[])
{
    signal(15, incache);
    pthread_t thid1 = 0, thid2 = 0, thid3 = 0;

    pthread_create(&thid1,  NULL, outcache,  NULL);
    pthread_create(&thid2,  NULL, outcache,  NULL);
    pthread_create(&thid3,  NULL, outcache,  NULL);

    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

void incache(int sig)
{
    static int id = 1;
    struct st_args stargs;
    memset(&stargs, 0, sizeof(struct st_args));

    // 锁住互斥锁
    pthread_mutex_lock(&mutex);

    // 生产数据，放入缓冲队列
    stargs.mesgid = id++; vcache.push_back(stargs);
    stargs.mesgid = id++; vcache.push_back(stargs);

    // 发送条件变量，解锁互斥锁
    pthread_mutex_unlock(&mutex);
    // pthread_cond_signal(&cond);
    pthread_cond_broadcast(&cond);
}

void *outcache(void *arg)
{
    struct st_args stargs;
    memset(&stargs, 0, sizeof(struct st_args));

    while (true)
    {
        // 锁住互斥锁
        pthread_mutex_lock(&mutex);

        // 判断是否有产品可供消费
        while (vcache.size() == 0)
        {
            // 没有产品可供消费，解锁互斥锁，供生产者访问互斥锁，并生产产品
            pthread_cond_wait(&cond, &mutex);
        }

        // 消费
        // 从缓冲池中取出一条信息
        memcpy(&stargs, &vcache[0], sizeof(struct st_args));
        vcache.erase(vcache.begin());

        // 解锁互斥锁
        pthread_mutex_unlock(&mutex);

        printf("thid = %lu, mesgid = %d\n", pthread_self(), stargs.mesgid);
        usleep(100);
    }
}