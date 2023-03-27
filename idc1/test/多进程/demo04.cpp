/* 
 * 测试fork函数的使用
 * 
 * 一个进程在fork后会创建一个新的子进程。子进程与父进程都会执行fork函数之后的代码
 * fork函数调用一次，返回两次。
 *  如果是父进程，返回0。如果是子进程，返回子进程的pid(编号)
 *  可以在程序中进行对父进程和子进程执行任务的处理
 *
 * 子进程获取了父进程的副本，而不是共享父进程的数据
 *  子进程获取了父进程的数据空间、堆和栈的副本
 *  父进程中打开的文件描述符也被复制到了子进程中
 *      文件中被写入的内容:
 *          我是一名互联网从业者
 *          bbb 我是一名互联网从业者
 *          我是一名互联网从业者
 *          aaa 我是一名互联网从业者
 *      "我是一名互联网从业者"出现了两次，但是它是在fork函数出现之前被写入到文件当中的。
 *      由于要写入的数据太少，fprintf会先将内容写入到缓冲区中，在进行fork函数之后，生成的子进程复制了父进程缓冲区的内容，并且也将内容写入到了文件中，因此会出现两次
 *      可以通过刷新缓冲区fflush函数来解决这个问题
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    FILE *fp = fopen("/tmp/test2.txt", "w+");
    fprintf(fp, "我是一名互联网从业者\n");
    fflush(fp);

    int ii = 0;
    int pid = fork();

    if (pid == 0)
    {
        printf("子进程%d\n", getpid());
        fprintf(fp, "bbb 我是一名互联网从业者\n");
        // printf("子进程：ii = %d\n", ii);
        // printf("子进程：ii = %d\n", ii);
        // printf("子进程：ii = %d\n", ii);
    }
    
    if (pid > 0)
    {
        printf("父进程%d\n", getpid());
        fprintf(fp, "aaa 我是一名互联网从业者\n");
        // printf("父进程：ii = %d\n", ii++);
        // printf("父进程：ii = %d\n", ii++);
        // printf("父进程：ii = %d\n", ii++);
    }

    fclose(fp);
    return 0;
}