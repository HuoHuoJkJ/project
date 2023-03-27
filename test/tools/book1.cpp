#include "_public.h"

void EXIT(int sig)
{
    printf("\nsig = %d\n", sig);
    // exit(0);函数只会调用全局对象的析构函数，而不会调用局部对象的析构函数
    // 而main函数中的return会调用全部对象的析构函数
    if (sig == 2)
        exit(0);
    // return ;
}

CPActive Active;

int main(int argc, char *argv[])
{
    if (argc < 3) { printf("Use:./book1 procname timeout\n"); return -1; }

    signal(2, EXIT); signal(15, EXIT);

    Active.AddPInfo(atoi(argv[2]), argv[1]);
    
    while (true)
    {
        // Active.UptATime();

        sleep(10);
    }

    return 0;
}