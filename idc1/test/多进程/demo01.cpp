/* 
 * 用于测试signal函数捕获Linux系统的信号，并进行信号的处理
 * 一般用于处理终止信号。我们不希望系统直接杀掉程序，而是希望在系统杀掉程序之前，处理一些善后的事情。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void func(int sig)
{
    printf("接收到了%d信号\n", sig);
}

void alarmfunc(int sig)
{
    printf("接收到了时钟信号%d\n", sig);
    alarm(3);
}

int main(void)
{
    for (int ii = 0; ii < 64; ii++)
        signal(ii,func);
    
/*     
    // SIG_IGN忽略信号
    signal(15, SIG_IGN);
    // SIG_DFL执行信号
    signal(15, SIG_DFL);
*/
    signal(SIGALRM, alarmfunc);
    // 在程序执行的第三秒执行一次，之后就不会再执行了
    alarm(3);

    while(true)
    {
        printf("这是一条正在执行的数据。\n");
        sleep(1);
    }

    return 0;
}