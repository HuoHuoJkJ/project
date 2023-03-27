/* 
 * 用于测试signal函数捕获Linux系统的信号，并进行信号的处理
 * 一般用于处理终止信号。我们不希望系统直接杀掉程序，而是希望在系统杀掉程序之前，处理一些善后的事情。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void EXIT(int sig)
{
    printf("接收到了%d信号，进程即将退出\n", sig);
    exit(0);
}

int main(void)
{
    for (int ii = 0; ii < 64; ii++)
        signal(ii,SIG_IGN);
    
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);
    
    while(true)
    {
        printf("这是一条正在执行的数据。\n");
        sleep(1);
    }

    return 0;
}