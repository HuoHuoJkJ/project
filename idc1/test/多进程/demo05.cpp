/* 
 * 测试fork函数的使用
 * 
 * 如果父进程先退出，子进程会变成孤儿进程，并被1号进程收养，有操作系统来处理子进程
 * 如果子进程先退出，内核向父进程发送SIGCHLD信号，如果父进程不处理这个信号，子进程会成为僵尸进程。
 *  如果子进程在父进程之前终止，内核为每个子进程保留了一个数据结构，包括进程编号、终止状态和使用cpu时间等父进程如果处理了子进程退出的信息，内核就会释放这个数据结构，
 *  如果父进程没有处理子进程退出的信息，内核就不会释放这个数据结构，子进程进程编号就会一直被占用，但是系统可用的进程号是有限的，如果大量的产生僵死进程，
 *  将因为没有可用的进程号而导致系统不能产生新的进程，这就是僵尸进程的危害。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void func(int sig)
{
    // signal 会通知系统内核，当产生SIGCHLD(子进程退出但父进程仍在时)时，会软中断父进程sleep(10);，来执行func的代码。使得父进程看上去和子进程一起退出
    // 让父进程再sleep(10);就可以不会一起退出了
    int sts;
    wait(&sts);
}

int main(void)
{
    // 忽略系统向父进程发出的信号SIGCHLD，这样可以避免产生僵尸进程
    // signal(SIGCHLD, SIG_IGN);
    signal(SIGCHLD, func);
    int pid = fork();

    if (pid == 0)
    {
        printf("子进程%d\n", getpid());
        sleep(5);
    }

    if (pid > 0)
    {
        printf("父进程%d\n", getpid());
        // 在子进程执行完毕前，wait函数会一直阻塞在此处
        // int sts;
        // wait(&sts);
        sleep(10);
        sleep(10);
    }

    return 0;
}