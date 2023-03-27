/* 
 * server01.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
 */
#include "../_public.h"

CLogFile    logfile;        // 服务器日志文件
CTcpServer  TcpServer;      // 创建服务对象

bool logstatus = false;

bool _main(char *strrecvbuffer, char *strsendbuffer);       // 业务处理主函数
bool srv000(char *strrecvbuffer, char *strsendbuffer);      // 心跳，保持客户端与服务端连接
bool srv001(char *strrecvbuffer, char *strsendbuffer);      // 登录业务
bool srv002(char *strrecvbuffer, char *strsendbuffer);      // 查询余额业务

void FathEXIT(int sig);     // 父进程的退出信号
void ChldEXIT(int sig);     // 子进程的退出信号

int main(int argc, char *argv[])
{
    if (argc != 4)
    { printf("Use:./server01 port logfile timeout\nExample:./server01 5005 /tmp/server01.log 35\n"); return -1; }
 
    // 关闭标准输入输出和信号，只保留信号2(SIGINT)和信号15(SIGTERM)
    CloseIOAndSignal(true); signal(SIGINT, FathEXIT); signal(SIGTERM, FathEXIT);

    if ( logfile.Open(argv[2], "w+") == false ) { printf("打开日志文件失败！\n"); return -1; }

    // signal(SIGCHLD, SIG_IGN);

    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { logfile.Write("服务器设置端口失败！\n"); return -1; }
    
    while (true)
    {
        if ( TcpServer.Accept() == false )
        { printf("Accept() failed!\n"); return -1; }
        logfile.Write("客户端(%s)连接成功！\n", TcpServer.GetIP());

        if (fork() > 0) { TcpServer.CloseClient(); continue; }

        signal(SIGINT, ChldEXIT); signal(SIGTERM, ChldEXIT);

        TcpServer.CloseListen();

        char strrecvbuffer[1024], strsendbuffer[1024];
        while (1)
        {
            memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
            memset(strsendbuffer, 0, sizeof(strsendbuffer));

            if ( TcpServer.Read(strrecvbuffer, atoi(argv[3])) == false ) break;
            logfile.Write("接受：%s\n", strrecvbuffer);

            // 处理银行业务
            if ( _main(strrecvbuffer, strsendbuffer) == false ) break;

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

// 业务处理主函数
bool _main(char *strrecvbuffer, char *strsendbuffer)
{
    // 解析strrecvbuffer，获取服务代码(业务代码)。
    int isrecode = -1;
    GetXMLBuffer(strrecvbuffer, "srvcode", &isrecode);
    
    if ( (isrecode != 1) && (logstatus == false) && (isrecode != 0) )
    { sprintf(strsendbuffer, "<retcode>-1</retcode><message>未登录！</message>"); return true; }
    
    // 处理每种业务
    switch(isrecode)
    {
        case 0:
            srv000(strrecvbuffer, strsendbuffer); break;
        case 1:
            srv001(strrecvbuffer, strsendbuffer); break;
        case 2:
            srv002(strrecvbuffer, strsendbuffer); break;
        default:
            logfile.Write("业务代码不合法：%s\n", strrecvbuffer);
            return false;
    }

    return true;
}

// 心跳，保持客户端与服务端连接
bool srv000(char *strrecvbuffer, char *strsendbuffer)
{
    strcpy(strsendbuffer, "<retcode>0</retcode><message>成功</message>");
    return true;
}

// 登录
bool srv001(char *strrecvbuffer, char *strsendbuffer)
{
    char tel[21], password[31];
    GetXMLBuffer(strrecvbuffer, "tel", tel, 20);
    GetXMLBuffer(strrecvbuffer, "password", password, 30);
    
    if ( strcmp(tel, "18561860345") == 0 && strcmp(password, "123456") == 0 )
    {
        strcpy(strsendbuffer, "<retcode>0</retcode><message>登录成功！</message>");
        logstatus = true;
    }
    else
        strcpy(strsendbuffer, "<retcode>-1</retcode><message>登录失败！</message>");

    return true;
}

// 查询余额
bool srv002(char *strrecvbuffer, char *strsendbuffer)
{
    char cardid[31];
    GetXMLBuffer(strrecvbuffer, "cardid", cardid, 30);
    
    if ( strcmp(cardid, "3123129412312") == 0 )
        strcpy(strsendbuffer, "<retcode>0</retcode><message>查询余额成功</message><ua>100.239</ua>");
    else
        strcpy(strsendbuffer, "<retcode>-1</retcode><message>查询余额失败</message>");

    return true;
}