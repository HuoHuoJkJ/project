#include "../_public.h"

CLogFile logfile;
CTcpServer TcpServer;

char strsendbuffer[1024];
char strrecvbuffer[1024];
bool islogin= false;

bool _main();
bool srv000();      // 心跳
bool srv001();      // 登录
bool srv002();      // 查询余额
void FATHEXIT(int sig);
void CHILEXIT(int sig);

int main(int argc, char *argv[])
{
    if (argc != 3) { printf("Use:./server port logfile\nExample:./server 5005 /project/public/socket2/server01.log\n"); return -1; }
    
    CloseIOAndSignal(true); signal(SIGINT, FATHEXIT); signal(SIGTERM, FATHEXIT);

    if (logfile.Open(argv[2], "a+") == false) { printf("日志文件(%s)失败\n", argv[2]); return -1; }

    if (TcpServer.InitServer(atoi(argv[1])) == false) { logfile.Write("InitServer() 失败\n"); return -1; }
    
    signal(SIGCHLD, SIG_IGN); // 忽略处理子进程结束的信号

    while (true)
    {
        if (TcpServer.Accept() == false) { logfile.Write("Accept() 失败\n"); FATHEXIT(-1); }

        if (fork() > 0) { TcpServer.CloseClient(); continue; }

        signal(SIGINT, CHILEXIT); signal(SIGTERM, CHILEXIT);
        TcpServer.CloseListen();

        logfile.Write("进程(%d)已连接客户端(%s)\n", getpid(), TcpServer.GetIP());

        while (true)
        {
            memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
            if (TcpServer.Read(strrecvbuffer, 10) == false) return false;
            logfile.Write("接收:%s\n", strrecvbuffer);

            _main();

            if (TcpServer.Write(strsendbuffer) == false) return false;
            logfile.Write("发送:%s\n", strsendbuffer);
        }

        CHILEXIT(0);
    }

    return 0;
}

bool _main()
{

    int  srvcode;
    GetXMLBuffer(strrecvbuffer, "srvcode", &srvcode);
    
    if (srvcode != 1 && islogin == false && srvcode != 0) { strcpy(strsendbuffer, "<retcode>-1</retcode><message>请先登录</message>"); return true; }

    switch (srvcode)
    {
    case 0:
        srv000();
        break;
    case 1:
        srv001();
        break;
    case 2:
        srv002();
        break;
    default:
        logfile.Write("发送的业务代码(%d)不合法\n", strrecvbuffer);
        return false;
    }
    return true;
}

bool srv000()
{
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<retcode>0</retcode><message>hahajiukanjian</message>");
    return true;
}

// 登录
bool srv001()
{
    char username[51];  memset(username, 0, sizeof(username));
    char password[51];  memset(password, 0, sizeof(password));
    GetXMLBuffer(strrecvbuffer, "username", username, 50);
    GetXMLBuffer(strrecvbuffer, "password", password, 50);

    if (strcmp(username, "hahajiukanjian") != 0 || strcmp(password, "dyt123") != 0)
        STRCPY(strsendbuffer, sizeof(strsendbuffer), "<retcode>-1</retcode>");
    else
    { STRCPY(strsendbuffer, sizeof(strsendbuffer), "<retcode>0</retcode>"); islogin = true; }

    return true;
}

// 查询余额
bool srv002()
{
    char cardid[51];    memset(cardid, 0, sizeof(cardid));
    GetXMLBuffer(strrecvbuffer, "cardid", cardid, 50);

    if (strcmp(cardid, "1695259395") != 0)
        STRCPY(strsendbuffer, sizeof(strsendbuffer), "<retcode>-1</retcode>");
    else
        STRCPY(strsendbuffer, sizeof(strsendbuffer), "<retcode>0</retcode><ye>100.10</ye>");

    return true;
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