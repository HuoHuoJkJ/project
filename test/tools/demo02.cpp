/* 
 * 测试if else是否都会执行
 */
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
using namespace std;

struct st_pid
{
    int pid;
    char name[51];
};

int main(void)
{
    struct st_pid *stpid = 0;
    cout << "stpid = " << stpid << endl;
    while (true)
    {
        if (fork() == 0)
        {
            printf("aaaaa\n"); sleep(2);
            execl("/usr/bin/pwd", "/usr/bin/pwd", (char *)0);
        }
        else
        {
            int sts;
            wait(&sts);
            sleep(10);
            printf("bbbbb\n"); sleep(1);
        }
    }

    return 0;
}