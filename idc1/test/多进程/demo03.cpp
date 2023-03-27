/* 
 * 测试fork函数的使用
 * 
 * 一个进程在fork后会创建一个新的子进程。子进程与父进程都会执行fork函数之后的代码
 * fork函数调用一次，返回两次。
 *  如果是父进程，返回0。如果是子进程，返回子进程的pid(编号)
 *  可以在程序中进行对父进程和子进程执行任务的处理
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    printf("aaa1 = %d\n", getpid());
    sleep(5);
    printf("bbb1 = %d\n", getpid());
    
    int pid = fork();
    printf("\npid = %d\n", pid);
    
    printf("ccc1 = %d\n", getpid());
    sleep(10);
    printf("ddd1 = %d\n", getpid());
    
    if (pid > 0)
    {
        printf("父进程%d\n", getpid());
    }

    if (pid == 0)
    {
        printf("子进程%d\n", getpid());
    }

    return 0;
}