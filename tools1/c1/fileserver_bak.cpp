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
bool _recvfiles(const int sockfd, const char *filename, const int filesize, const char *ptime);
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
            if (_recvfilesmain() == false) { logfile.Write("处理客户端信息失败\n"); _chilexit(-1); }
        }

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
        // logfile.Write("接收--客户信息：%s\n", recvbuffer);

        // 处理心跳报文
        if (strstr(recvbuffer, "<activetest>") != 0)
        {
            logfile.Write("接收--客户信息：%s\n", recvbuffer);
            strcpy(sendbuffer, "ok");
            logfile.Write("发送--回应信息：%s\n", sendbuffer);
        }
 
        // 处理文件信息
        if (strstr(recvbuffer, "<filename>") != 0)
        {
            // 解析xml字符串，文件名，文件大小，修改时间
            char clientfilename[301]; memset(clientfilename, 0, sizeof(clientfilename));
            char ptime[51];           memset(ptime, 0, sizeof(ptime));
            int  filesize = 0;
            GetXMLBuffer(recvbuffer, "filename", clientfilename, 300);
            GetXMLBuffer(recvbuffer, "ptime", ptime, 50);
            GetXMLBuffer(recvbuffer, "psize", &filesize);
            
            // 客户端和服务端的文件名的绝对路径是不同的，我们应该进行替换，变换出服务端的文件名
            char serverfilename[301]; memset(serverfilename, 0, sizeof(serverfilename));
            strcpy(serverfilename, clientfilename);
            UpdateStr(serverfilename, starg.clientpath, starg.serverpath, false);

            // 接收上传文件的内容
            // 将接收的文件信息拼接成报文
            logfile.Write("接收 %s(%d) ... ", serverfilename, filesize);
            if (_recvfiles(TcpServer.m_connfd, serverfilename, filesize, ptime) == false)
            {
                logfile.WriteEx("失败\n");
                SPRINTF(sendbuffer, sizeof(sendbuffer), "<filename>%s</filename><result>failed</result>", clientfilename);
                return false;
            }
            logfile.WriteEx("成功\n");
            SPRINTF(sendbuffer, sizeof(sendbuffer), "<filename>%s</filename><result>success</result>", clientfilename);
        }

        // 回应客户端
        if (TcpServer.Write(sendbuffer) == false) return false;
        // logfile.Write("发送--回应信息：%s\n", sendbuffer);
    }

    return true;
}

bool _recvfiles(const int sockfd, const char *filename, const int filesize, const char *ptime)
{
    int  onrean = 0;
    int  totalbytes = 0;
    char buffer[1000];
    FILE *fp = NULL;
    char filenametmp[301];
    // 创建临时文件名
    SPRINTF(filenametmp, sizeof(filenametmp), "%s.tmp", filename);

    // 创建临时文件
    if ((fp=FOPEN(filenametmp, "wb")) == NULL)
    { logfile.Write("创建临时文件(%s)失败\n", filenametmp); return false; }
    
    while (totalbytes < filesize)
    {
        memset(buffer, 0, sizeof(buffer));
        // 计算本次应该读取的字节数，如果剩余的数据超过1000字节，就打算读1000字节
        if ((onrean = (filesize-totalbytes))<1000);
        else onrean = 1000;

        // 接收文件内容
        if (Readn(sockfd, buffer, onrean) == false) { logfile.Write("客户端断开连接，接收文件内容失败\n"); fclose(fp); return false; }

        // 把接收到的内容写入临时文件
        fwrite(buffer, 1, onrean, fp);

        // 计算已经接收的总字节数
        totalbytes = totalbytes + onrean;
    }

    // 关闭临时文件
    fclose(fp);

    // 重置文件时间 如果没有重置文件的时间，那么filename的文件时间为传输文件的时间，而不是客户端中原始文件的时间。我们认为原始文件的时间更有意义。
    UTime(filenametmp, ptime);

    // 把临时文件名改为正式文件名
    RENAME(filenametmp, filename);

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