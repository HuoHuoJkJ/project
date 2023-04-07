#include "_public.h"

struct st_arg
{
    int  clienttype;
    char ip[31];
    int  port;
    int  ptype;
    char clientpath[301];
    char serverpath[301];
    char clientpathbak[301];
    char matchname[301];
    bool andchild;
    int  timetvl;
    int  timeout;
    char pname[51];
} starg;

CLogFile    logfile;
CTcpServer  TcpServer;

char sendbuffer[1024];
char recvbuffer[1024];

bool _recvxml();
bool _xmltoarg(const char *buffer);
bool _recvfilesmain();
bool _recvfiles();
void _fathexit(int sig);
void _chilexit(int sig);

int main(int argc, char *argv[])
{
    // 编写帮助文档
    if (argc != 3)
    {
        printf("Use:fileserver logfile port\nExample:/project/tools1/bin/fileserver /log/tools/fileserver1.log 5005\n");
        return -1;
    }

    // 关闭IO和信号，只处理2和15信号
    CloseIOAndSignal(true); signal(SIGINT, _fathexit); signal(SIGTERM, _fathexit);

    // 打开日志文件
    if (logfile.Open(argv[1], "a+") == false) { printf("打开日志文件(%s)失败\n", argv[1]); return -1; }

    if (TcpServer.InitServer(atoi(argv[2])) == false) { logfile.Write("初始化Tcp服务器失败\n"); _fathexit(-1); }
    
    while (true)
    {
        if (TcpServer.Accept() == false)
        { break; }

        /*
        if (fork() > 0)
        { TcpServer.CloseClient(); continue; }

        signal(SIGINT, _chilexit); signal(SIGTERM, _chilexit);
        TcpServer.CloseListen();
        */

        logfile.Write("进程(%d)已连接客户端(%s)\n", getpid(), TcpServer.GetIP());

        if (_recvxml() == false) _chilexit(-1);

        if (starg.clienttype == 1)
        {
            if (_recvfilesmain() == false) { logfile.Write("获取客户端文件失败\n"); _chilexit(-1); }
        }

        logfile.Write("\n\n\n\n\n\n");
        _chilexit(0);
    }

    return 0;
}

bool _recvxml()
{
    memset(sendbuffer, 0, sizeof(sendbuffer));
    memset(recvbuffer, 0, sizeof(recvbuffer));
    
    if (TcpServer.Read(recvbuffer, 20) == false) return false;
    logfile.Write("接收--确认参数：%s\n", recvbuffer);

    if (_xmltoarg(recvbuffer) == false) 
        SPRINTF(sendbuffer, sizeof(sendbuffer), "failed");
    else
        SPRINTF(sendbuffer, sizeof(sendbuffer), "ok");

    if (TcpServer.Write(sendbuffer) == false) return false;
    logfile.Write("发送--回应参数：%s\n", sendbuffer);

    logfile.Write("信息--确认结果：%s:%s\n", TcpServer.GetIP(), sendbuffer);

    return true;
}

bool _xmltoarg(const char *buffer)
{
    memset(&starg, 0, sizeof(starg));

    GetXMLBuffer(buffer, "clienttype", &starg.clienttype);
    if ( (starg.clienttype != 1) && (starg.clienttype != 2) )
    { logfile.Write("clienttype的值不正确, clienttype=%d\n", starg.clienttype); return false; }
    GetXMLBuffer(buffer, "ptype", &starg.ptype);
    GetXMLBuffer(buffer, "clientpath", starg.clientpath, 300);
    GetXMLBuffer(buffer, "serverpath", starg.serverpath, 300);
    GetXMLBuffer(buffer, "clientpathbak", starg.clientpathbak, 300);
    GetXMLBuffer(buffer, "matchname", starg.matchname, 300);
    GetXMLBuffer(buffer, "andchild", &starg.andchild);
    GetXMLBuffer(buffer, "timetvl", &starg.timetvl);;
    if ((starg.timetvl < 0) && (starg.timetvl >= 30))
    { logfile.Write("timetvl的值不符合规范，已改为默认值30\n"); starg.timetvl = 30; }
    GetXMLBuffer(buffer, "timeout", &starg.timeout);;
    if ((starg.timeout < 50) && (starg.timeout >= 120))
    { logfile.Write("timetvl的值不符合规范，已改为默认值50\n"); starg.timetvl = 50; }
    GetXMLBuffer(buffer, "pname", starg.pname, 50);
    strcat(starg.pname, "_srv");

    return true;
}

bool _recvfilesmain()
{
    while (true)
    {
        memset(sendbuffer, 0, sizeof(sendbuffer));
        memset(recvbuffer, 0, sizeof(recvbuffer));
        // 接收心跳报文 或者 接收发送过来的文件信息
        if (TcpServer.Read(recvbuffer, starg.timetvl+5) == false) return false;
        logfile.Write("接收--客户信息：%s\n", recvbuffer);
        
        // 处理心跳报文
        if (strstr(recvbuffer, "<activetest>") != 0)
        {
            strcpy(sendbuffer, "ok");
        }
        
        // 处理文件信息
        if (strstr(recvbuffer, "<filename>") != 0)
        {
            logfile.Write("接收--文件信息：%s\n", recvbuffer);
            strcpy(sendbuffer, "ok");
            // _recvfiles();
        }

        if (TcpServer.Write(sendbuffer) == false) return false;
        logfile.Write("发送--回应信息：%s\n", sendbuffer);
    }

    return true;
}

bool _recvfiles()
{


    return true;
}

void _fathexit(int sig)
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);
    logfile.Write("父进程退出，sig=%d\n", sig);
    TcpServer.CloseListen();
    kill(0, 15);
    exit(0);
}

void _chilexit(int sig)
{
    logfile.Write("子进程退出，sig=%d\n", sig);
    TcpServer.CloseClient();
    exit(0);
}