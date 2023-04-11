#include "_public.h"

struct st_arg
{
    int  clienttype;
    char ip[31];
    int  port;
    int  ptype;
    char clientpath[301];
    char serverpath[301];
    char serverpathbak[301];
    char matchname[301];
    bool andchild;
    int  timetvl;
    int  timeout;
    char pname[51];
} starg;

CLogFile    logfile;
CTcpClient  TcpClient;
CPActive    PActive;

char sendbuffer[1024];
char recvbuffer[1024];

void _help();
bool _xmltoarg(const char *buffer);
bool _sendxml(const char *argv);
bool _recvfilesmain();
bool _recvfiles(const int sockfd, const char *fullfilename, const int filesize, const char *filetime);
bool _removeorbak();
void _exit(int sig);

int main(int argc, char *argv[])
{
    // 编写帮助文档
    if (argc != 3) { _help(); return -1; }

    // 关闭IO和信号，只处理2和15信号
    CloseIOAndSignal(true); signal(SIGINT, _exit); signal(SIGTERM, _exit);

    // 打开日志文件
    if (logfile.Open(argv[1], "a+") == false) { printf("打开日志文件(%s)失败\n", argv[1]); _exit(-1); }
    
    // 解析xml字符串
    if (_xmltoarg(argv[2]) == false) { logfile.Write("解析xml字符串(%s)失败\n", argv[2]); _exit(-1); }

    PActive.AddPInfo(starg.timeout, starg.pname);

    // 连接tcp服务器
    TcpClient.ConnectToServer(starg.ip, starg.port);

    // 向服务端确认连接参数
    if (_sendxml(argv[2]) == false) { logfile.Write("向服务端传输参数失败\n"); _exit(-1); }

    // 发送文件信息和内容
    _recvfilesmain();

    _exit(0);
}

void _help()
{
    printf("Use:tcpgetfiles logfile xmlbuffer\n");    
    printf("Example:/project/tools1/bin/tcpgetfiles /log/tools/tcpgetfiles.log \"<ip>10.0.8.4</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcp/client</clientpath><serverpath>/tmp/tcp/server</serverpath><serverpathbak>/tmp/tcp/serverbak</serverpathbak><matchname>*.JSON,*.XML</matchname><andchild>true</andchild><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles</pname>\"\n");    
}

bool _xmltoarg(const char *buffer)
{
    memset(&starg, 0, sizeof(starg));

    GetXMLBuffer(buffer, "ip", starg.ip, 30);
    if (strlen(starg.ip) == 0) { logfile.Write("ip的值为空\n"); return false; }

    GetXMLBuffer(buffer, "port", &starg.port);
    if (starg.port <= 0) { logfile.Write("port的值不正确, port=%d\n", starg.port); return false; }

    GetXMLBuffer(buffer, "ptype", &starg.ptype);
    if ((starg.ptype != 1) && (starg.ptype != 2)) { logfile.Write("ptype的值不正确, ptype=%d\n", starg.ptype); return false; }

    GetXMLBuffer(buffer, "clientpath", starg.clientpath, 300);
    if (strlen(starg.clientpath) == 0) { logfile.Write("clientpath的值为空\n"); return false; }

    GetXMLBuffer(buffer, "serverpath", starg.serverpath, 300);
    if (strlen(starg.serverpath) == 0) { logfile.Write("serverpath的值为空\n"); return false; }

    GetXMLBuffer(buffer, "serverpathbak", starg.serverpathbak, 300);
    if (starg.ptype == 2 && strlen(starg.serverpathbak) == 0) { logfile.Write("serverpathbak的值为空\n"); return false; }

    GetXMLBuffer(buffer, "matchname", starg.matchname, 300);
    if (strlen(starg.matchname) == 0) { logfile.Write("matchname的值为空\n"); return false; }

    GetXMLBuffer(buffer, "andchild", &starg.andchild);

    GetXMLBuffer(buffer, "timetvl", &starg.timetvl);;
    if ((starg.timetvl < 0) && (starg.timetvl >= 30))
    { logfile.Write("timetvl的值不符合规范，已改为默认值30\n"); starg.timetvl = 30; }

    GetXMLBuffer(buffer, "timeout", &starg.timeout);;
    if ((starg.timeout < 50) && (starg.timeout >= 120))
    { logfile.Write("timetvl的值不符合规范，已改为默认值50\n"); starg.timetvl = 50; }

    GetXMLBuffer(buffer, "pname", starg.pname, 50);
    if (strlen(starg.pname) == 0) { logfile.Write("pname的值为空\n"); return false; }

    return true;
}

bool _sendxml(const char *argv)
{
    memset(sendbuffer, 0, sizeof(sendbuffer));
    memset(recvbuffer, 0, sizeof(recvbuffer));
    
    SPRINTF(sendbuffer, sizeof(sendbuffer), "<clienttype>2</clienttype>%s", argv);
    if (TcpClient.Write(sendbuffer) == false) return false;
    logfile.Write("发送--确认参数：%s\n", sendbuffer);

    if (TcpClient.Read(recvbuffer, 20) == false) return false;
    logfile.Write("接收--回应参数：%s\n", recvbuffer);

    logfile.Write("信息--连接成功：%s:%d\n", starg.ip, starg.port);

    return true;
}

bool _recvfilesmain()
{
    while (true)
    {
        memset(recvbuffer, 0, sizeof(recvbuffer));
        memset(sendbuffer, 0, sizeof(sendbuffer));
        PActive.UptATime();
        // 接收服务端发送的文件信息报文
        if (TcpClient.Read(recvbuffer, starg.timetvl+10) == false)
        { logfile.Write("接收----服务发送报文：%s 失败\n", recvbuffer); return false; }
        // logfile.Write("接收----服务发送报文：%s\n", recvbuffer);

        if (strstr(recvbuffer, "<activetest>") != 0)
        {
            logfile.Write("<activetest>success</activetest>\n");
            SPRINTF(sendbuffer, sizeof(sendbuffer), "<activetest>success</activetest>");
        }

        if (strstr(recvbuffer, "<filename>") != 0)
        {
            // 处理文件信息报文
            char serverfilename[301];   memset(serverfilename, 0, sizeof(serverfilename));
            char filetime[21];          memset(filetime, 0, sizeof(filetime));
            int  filesize = 0;
            GetXMLBuffer(recvbuffer, "filename", serverfilename, 300);
            GetXMLBuffer(recvbuffer, "ptime", filetime, 20);
            GetXMLBuffer(recvbuffer, "psize", &filesize);
            
            // 生成客户端文件名的字符串
            char clientfilename[301];
            STRCPY(clientfilename, sizeof(clientfilename), serverfilename);
            UpdateStr(clientfilename, starg.serverpath, starg.clientpath, false);
            
            // 接收文件内容 拼接返回报文
            logfile.Write("接收文件 %s ... ", clientfilename);
            if (_recvfiles(TcpClient.m_connfd, clientfilename, filesize, filetime) == false)
            {
                logfile.WriteEx("失败\n");
                SPRINTF(sendbuffer, sizeof(sendbuffer), "<filename>%s</filename><result>failed</result>", serverfilename);
            }
            logfile.WriteEx("成功\n");
            SPRINTF(sendbuffer, sizeof(sendbuffer), "<filename>%s</filename><result>success</result>", serverfilename);
        }
        if (TcpClient.Write(sendbuffer) == false)
        { logfile.Write("发送----客户回应报文：%s 失败\n", sendbuffer); return false; }
        // logfile.Write("发送----客户回应报文：%s\n", sendbuffer);
    }
    return true;
}

bool _recvfiles(const int sockfd, const char *fullfilename, const int filesize, const char *filetime)
{
    int  totalbytes = 0;
    int  onread     = 0;
    FILE *fp        = NULL;
    char buffer[1000];
    char tmpfilename[301];
    // 生成临时文件名的字符串
    SPRINTF(tmpfilename, sizeof(tmpfilename), "%s.tmp", fullfilename);

    // 创建临时文件 FOPEN
    if ((fp=FOPEN(tmpfilename, "wb")) == NULL) { logfile.Write("打开临时文件失败\n"); return false; }
    
    // 若已读取的总字节数与文件大小相等，跳出循环
    while (totalbytes < filesize)
    {
        memset(buffer, 0, sizeof(buffer));
        // 计算本次待读取的字节数，如果大于1000，就读取1000
        if    ((onread = (filesize-totalbytes)) < 1000);
        else    onread = 1000;

        // 接收文件内容
        if (Readn(sockfd, buffer, onread) == false)
        { logfile.Write("接收文件%s内容失败\n", fullfilename); fclose(fp); return false; }

        // 写入到临时文件中
        fwrite(buffer, 1, onread, fp);

        // 计算已读取的总字节数
        totalbytes = totalbytes + onread;
    }

    // 关闭临时文件
    fclose(fp);

    // 更新文件时间
    UTime(tmpfilename, filetime);

    // 更名为正式文件
    if (RENAME(tmpfilename, fullfilename) == false) return false;

    return true;
}

void _exit(int sig)
{
    logfile.Write("程序即将推出，sig=%d\n", sig);

    exit(0);
}