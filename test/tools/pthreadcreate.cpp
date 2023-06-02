#include "_public.h"

void *thmain(void *arg)
{
    sleep(20);
}

int main(void)
{
    pthread_t thid = 0;
    for (int ii = 0; ii < 10000; ii ++)
    {
        if (pthread_create(&thid, NULL, thmain, (void *)(long)(0)) != 0)
        {
            printf("create pthread failed!\n");
            return -1;
        }
    }
    printf("ok\n");
    sleep(10);

    return 0;
}