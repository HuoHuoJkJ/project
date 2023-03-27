/* 
 * server11.cpp 作为服务端，接受客户端的数据，用来演示同步通信的效率
 */
#include "../_public.h"

CLogFile    logfile;        // 服务器日志文件
CTcpServer  TcpServer;      // 创建服务对象

bool logstatus = false;

void FathEXIT(int sig);     // 父进程的退出信号
void ChldEXIT(int sig);     // 子进程的退出信号

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Use:./server11 port logfile\nExample:./server11 5005 /tmp/server11.log\n"); return -1; }
 
    // 关闭标准输入输出和信号，只保留信号2(SIGINT)和信号15(SIGTERM)
    CloseIOAndSignal(true); signal(SIGINT, FathEXIT); signal(SIGTERM, FathEXIT);

    if ( logfile.Open(argv[2], "w+") == false ) { printf("打开日志文件失败！\n"); return -1; }

    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { logfile.Write("服务器设置端口失败！\n"); return -1; }
    
    while (true)
    {
        if ( TcpServer.Accept() == false )
        { printf("Accept() failed!\n"); return -1; }
        logfile.Write("客户端(%s)连接成功！\n", TcpServer.GetIP());

        // 从这里fork()
        if (fork() > 0) { TcpServer.CloseClient(); continue; }
        // 子进程从这了开始
        signal(SIGINT, ChldEXIT); signal(SIGTERM, ChldEXIT);
        TcpServer.CloseListen();

        char strrecvbuffer[1024], strsendbuffer[1024];
        while (1)
        {
            memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
            memset(strsendbuffer, 0, sizeof(strsendbuffer));

            if ( TcpServer.Read(strrecvbuffer) == false ) break;
            logfile.Write("接受：%s\n", strrecvbuffer);

            SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1024, "ok");
            if ( TcpServer.Write(strsendbuffer) == false ) break;
            logfile.Write("发送：%s\n", strsendbuffer);
        }
        return 0;
    }

    return 0;
}

// 父进程的退出信号
void FathEXIT(int sig)     
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    logfile.Write("父进程(%d)退出！sig=%d\n", getpid(), sig);

    TcpServer.CloseListen();    // 先关闭监听的socket

    // 在Linux中，每个进程都有一个PID（Process ID），同时也会被分配到一个进程组ID（PGID）以及一个会话（Session）ID。
    // 一个进程组中包含了一个或多个进程，每个进程组都有一个进程组ID，进程组中的所有进程共享同一个终端（tty）。
    // 在一个会话中，一个进程组可以有一个或多个。 
    // 当使用kill函数发送信号时，如果传入的参数pid为正数，则表示要发送给指定进程ID的进程；
    // 如果pid为0，则表示将信号发送给与调用进程属于同一进程组的所有进程，即向同一进程组中的所有进程发送信号。
    // 因此，这里使用kill(0, 15)可以向同一进程组中的所有子进程发送SIGTERM信号，通知它们退出。
    kill(0, sig);                // 通知全部的子进程退出

    exit(0);
}

// 子进程的退出信号
void ChldEXIT(int sig)
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    logfile.Write("子进程(%d)退出！sig=%d\n", getpid(), sig);

    TcpServer.CloseClient();    // 先关闭客户端的socket

    exit(0);
}