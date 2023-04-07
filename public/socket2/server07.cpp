#include "../_public.h"

CLogFile logfile;
CTcpServer TcpServer;

void FATHEXIT(int sig);
void CHILEXIT(int sig);

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Use:./server port logfile\nExample:./server 5005 /project/public/socket2/server01.log\n"); return -1; }
    
    CloseIOAndSignal(true); signal(SIGINT, FATHEXIT); signal(SIGTERM, FATHEXIT);

    if (logfile.Open(argv[2], "a+") == false)
    { printf("日志文件(%s)失败\n", argv[2]); return -1; }

    if (TcpServer.InitServer(atoi(argv[1])) == false)
    {
        logfile.Write("InitServer() 失败\n");
        return -1;
    }
    
    signal(SIGCHLD, SIG_IGN); // 忽略处理子进程结束的信号

    while (true)
    {
        if (TcpServer.Accept() == false)
        {
            logfile.Write("Accept() 失败\n");
            FATHEXIT(-1);
        }

        if (fork() > 0)
        {
            TcpServer.CloseClient();
            continue;
        }
        signal(SIGINT, CHILEXIT); signal(SIGTERM, CHILEXIT);
        TcpServer.CloseListen();

        logfile.Write("进程(%d)已连接客户端(%s)\n", getpid(), TcpServer.GetIP());

        char buffer[1024];
        while (true)
        {
            memset(buffer, 0, sizeof(buffer));
            if (TcpServer.Read(buffer) == false) break;
            logfile.Write("接收:%s\n", buffer);

            STRCPY(buffer, sizeof(buffer), "已接收");
            if (TcpServer.Write(buffer) == false) break;
            logfile.Write("发送:%s\n", buffer);
        }
        CHILEXIT(0);
    }

    return 0;
}
void FATHEXIT(int sig)
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);
    logfile.Write("父进程退出，sig=%d\n", sig);
    TcpServer.CloseListen();
    kill(0, 15);
    exit(0);
}

void CHILEXIT(int sig)
{
    logfile.Write("子进程退出，sig=%d\n", sig);
    TcpServer.CloseClient();
    exit(0);
}