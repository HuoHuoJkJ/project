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
    char *pargv[argc];
    for (int i = 2; i < argc; i++)
        pargv[i-2] = argv[i];
    pargv[argc-2] = NULL;

    while(true)
    {
        if(fork() == 0)
        {
            execv(argv[2], pargv);
            // execl("/usr/bin/ls", "/usr/bin/ls", "-lt", "/tmp", (char *)0);
            // 这样需要每次都判断用户输入的参数，非常麻烦，可以使用execv函数来处理
            // if(argc = 5) execl(argv[2], argv[2], argv[3], argv[4], (char *)0);
        }
        else
        {
            int status;
            wait(&status);
            sleep(atoi(argv[1]));
        }
    }
    
    // exec是用参数指定的程序，替换了当前进程的报文段、数据段、堆和栈
    // printf("aaa\n");
    // execl("/usr/bin/ls", "/usr/bin/ls", "-lt", "/tmp", (char *)0);
    // int iret = execl("/usr/bin/sls", "/usr/bin/ls", "-lt", "/tmp", (char *)0);
    // execl("/usr/bin/pwd", "/ust/bin/pwd", (char *)0);
    // printf("bbb  iret = %d\n", iret);
    // printf("bbb\n");

    return 0;
}
