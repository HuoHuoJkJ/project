#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    /* 
     * 由于执行execl函数后，会杀死当前进程，这不是我们想看到的，因此我们可以调用fork函数，让子进程来执行execl函数，这样就能够保证父进程的执行
     * procctl 10 /usr/bin/ls -lt /tmp
     */
    if (argc < 3)
    {
        printf("Error! Use procctl timetvl program argv ...\n");
        printf("Example:/project/tools1/bin/procctl 5 /usr/bin/ls -lt /tmp /project\n\n");
        
        printf("本程序是服务程序的调度程序，周期性启动服务程序或shell脚本\n");
        printf("timetvl program的运行周期 单位：秒 被调度的服务程序在timetvl秒后会被本程序再次启动。\n");
        printf("program 被调度的服务程序名 必须使用全路径。\n");
        printf("argv ... 服务程序调用的参数。\n");
        printf("注意，本程序不会被kill杀死，但是可以被kill -9/-19强行杀死\n\n");
        return -1;
    }

    for (int i = 0; i < 64; i++)
    {
        signal(i, SIG_IGN);
        close(i);
    }
    
    // 退出父进程，让进程有1号进程托管。下面的代码由子进程来执行，子进程被fork后，也会产生子进程.......
    if(fork() > 0) exit(0);
    
    signal(SIGCHLD, SIG_DFL);

    char *pargv[argc];
    for (int i = 2; i < argc; i++)
        pargv[i-2] = argv[i];
    pargv[argc-2] = NULL;

    while(true)
    {
        if(fork() == 0)
        {
            // 因为如果execv函数执行成功，原本的进程会被替换成execv执行的进程，exit(0);不会被执行。
            execv(argv[2], pargv);
            // 如果execv函数执行失败，应当退出进程。
            exit(0);
        }
        else
        {
            int status;
            wait(&status);
            sleep(atoi(argv[1]));
        }
    }
    
    return 0;
}
