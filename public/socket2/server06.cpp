#include "../_public.h"

CLogFile logfile;

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Use:./server port logfile\nExample:./server 5005 /project/public/socket2/server01.log\n"); return -1; }
    
    if (logfile.Open(argv[2], "a+") == false)
    { printf("日志文件(%s)失败\n", argv[2]); return -1; }

    CTcpServer TcpServer;
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
            return -1;
        }

        if (fork() > 0)
        {
            TcpServer.CloseClient();
            continue;
        }
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
        return 0;
    }

    return 0;
}