#include "_public.h"

/* 
 * 帮助文档  日志文件  初始化服务端tcp  与客户端确认连接信息
 * while
 *      父进程accept
 *      子进程处理报文
 *          接收客户端的确认连接报文参数   处理参数，存储到starg中
 */

struct st_arg
{
    int  clienttype;
    char clientpath[301];
    char serverpath[301];
    int  ptype;
    char clientpathbak[301];
    bool andchild;
    char ip[21];
    int  port;
    char matchname[301];
    int  timetvl;
    int  timeout;
    char pname[51];
} starg;

char recvbuffer[1024];
char sendbuffer[1024];

CLogFile    logfile;
CTcpServer  TcpServer;

bool _RecvXmlBuffer();
bool _XmlToArg(const char *buffer);
bool _RecvFilesMain();
bool _RecvFiles(const int sockfd, const char *filename, const int filesize, const char *filetime);
void _FathExit(int sig);
void _ChldExit(int sig);

int main(int argc, char *argv[])
{
    // 帮助文档
    if (argc != 3) { printf("/project/tools1/bin/fileserver /log/tools/fileserver1.log 5005\n"); return -1; }

    // 关闭IO和信号
    CloseIOAndSignal(true); signal(SIGINT, _FathExit); signal(SIGTERM, _FathExit);
    
    // 日志文件
    if (logfile.Open(argv[1], "a+") == false) { printf("打开日志文件(%s)失败\n", argv[1]); return -1; }
    
    // 初始化服务器
    if (TcpServer.InitServer(atoi(argv[2])) == false) { logfile.Write("初始化服务器失败\n"); _FathExit(-1); }
    
    while (true)
    {
        // 接收客户端accept
        if (TcpServer.Accept() == false) { logfile.Write("连接(accept)客户端失败\n"); _FathExit(-1); }

        // // fork() 父进程继续accept 子进程处理客户端的报文信息
        // if (fork() > 0) { TcpServer.CloseClient(); continue; }

        // // 关闭子进程的listenfd  处理子进程的2和15信号
        // TcpServer.CloseListen();
        // signal(SIGINT, _ChldExit); signal(SIGTERM, _ChldExit);

        logfile.Write("进程(%d)已连接客户端(%s:%s)\n", getpid(), TcpServer.GetIP(), argv[2]);

        // 与客户端确认连接参数
        if (_RecvXmlBuffer() == false) _ChldExit(-1);

        // 确认客户端的请求模式，ptype=1 上传文件 ----- ptype=2 下载文件
        if (starg.clienttype == 1)
        {
            if (_RecvFilesMain() == false) _ChldExit(-1);
        }

        else if (starg.clienttype == 2)
        {
            ;
        }

        _ChldExit(0);
    }

    return 0;
}

bool _RecvXmlBuffer()
{
    memset(sendbuffer, 0, sizeof(sendbuffer));
    memset(recvbuffer, 0, sizeof(recvbuffer));

    if (TcpServer.Read(recvbuffer, starg.timetvl+10) == false) { logfile.Write("接收----确认参数报文：失败\n"); return false; }
    logfile.Write("接收----确认参数报文：%s\n", recvbuffer);
    
    if (_XmlToArg(recvbuffer) == false)
        SPRINTF(sendbuffer, sizeof(sendbuffer), "failed");
    else
        SPRINTF(sendbuffer, sizeof(sendbuffer), "ok");
    
    if (TcpServer.Write(sendbuffer) == false) { logfile.Write("发送----回应确认参数：失败\n"); return false; }
    logfile.Write("发送----回应确认参数：%s\n", sendbuffer);

    logfile.Write("信息--确认结果：%s:%s\n", TcpServer.GetIP(), sendbuffer);

    return true;
}

bool _XmlToArg(const char *buffer)
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

bool _RecvFilesMain()
{
    while (true)
    {
        memset(sendbuffer, 0, sizeof(sendbuffer));
        memset(recvbuffer, 0, sizeof(recvbuffer));
        if (TcpServer.Read(recvbuffer, starg.timetvl+10) == false) return false;
        
        if (strstr(recvbuffer, "<activetest>") != 0)
        {
            logfile.Write("接收----客户发送报文：%s\n", recvbuffer);
            SPRINTF(sendbuffer, sizeof(sendbuffer), "success");
            logfile.Write("发送----响应客户报文：%s\n", sendbuffer);
        }

        if (strstr(recvbuffer, "<filename>") != 0)
        {
            // 将客户端发送的文件信息保存到变量中
            char clientfilename[301];   memset(clientfilename, 0, sizeof(clientfilename));
            char filetime[21];          memset(filetime, 0, sizeof(filetime));
            int  filesize = 0;
            GetXMLBuffer(recvbuffer, "filename", clientfilename, 300);
            GetXMLBuffer(recvbuffer, "ptime", filetime, 20);
            GetXMLBuffer(recvbuffer, "psize", &filesize);

            // 创建上传到服务端的服务端文件路径名
            char serverfilename[301];
            STRCPY(serverfilename, sizeof(serverfilename), clientfilename);
            UpdateStr(serverfilename, starg.clientpath, starg.serverpath, false);

            // 接收文件信息并保存到客户端  拼接回应客户端的报文
            logfile.Write("接收 %s ... ", clientfilename);
            if (_RecvFiles(TcpServer.m_connfd, serverfilename, filesize, filetime) == false)
            {
                logfile.WriteEx("失败\n");
                SPRINTF(sendbuffer, sizeof(sendbuffer), "<filename>%s</filename><result>failed</result>", clientfilename);
            }
            logfile.WriteEx("成功\n");
            SPRINTF(sendbuffer, sizeof(sendbuffer), "<filename>%s</filename><result>success</result>", clientfilename);
        }
        
        // 回应客户端
        if (TcpServer.Write(sendbuffer) == false) return false;
    }

    return true;
}

bool _RecvFiles(const int sockfd, const char *filename, const int filesize, const char *filetime)
{
    int  onread = 0;
    int  totalbytes = 0;
    char buffer[1000];
    FILE *fp = NULL;
    char tmpfilename[301];
    // 创建临时文件名
    STRCPY(tmpfilename, sizeof(tmpfilename), filename);
    UpdateStr(tmpfilename, starg.serverpath, starg.clientpathbak, false);

    // 以wb的形式打开临时文件
    if ((fp = FOPEN(tmpfilename, "wb")) == NULL) { logfile.Write("打开临时文件失败\n"); return false; }
    
    while (totalbytes < filesize)
    {
        // 确定本次读取的字节数，大于1000 就读取1000字节
        if   ((onread = filesize - totalbytes) < 1000);
        else onread = 1000;

        // 读取客户端发送的二进制流
        if (Readn(sockfd, buffer, onread) == false) return false;

        // 写入到临时文件中
        fwrite(buffer, 1, onread, fp);

        // 计算已读取的总字节数
        totalbytes = totalbytes + onread;
    }
    
    // 关闭临时文件
    fclose(fp);

    // 更新临时文件时间
    UTime(tmpfilename, filetime);

    // 更名为正式文件
    if (RENAME(tmpfilename, filename) == false) { logfile.Write("重命名文件(%s--%s)失败\n", tmpfilename, filename); return false; }

    return true;
}

void _FathExit(int sig)
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);
    logfile.Write("父进程(%d)即将退出，sig=%d\n", getpid(), sig);
    TcpServer.CloseListen();
    kill(0, 15);
    exit(0);
}

void _ChldExit(int sig)
{
    logfile.Write("子进程(%d)即将退出，sig=%d\n", getpid(), sig);
    TcpServer.CloseClient();
    exit(0);
}