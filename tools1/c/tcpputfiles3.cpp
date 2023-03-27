/* 
 * 程序名：tcpputfiles.cpp  基于TCP协议上传文件至服务端
 */
#include "_public.h"

struct st_arg
{
    int  clienttype;            // 客户端类型。1-上传文件；2-下载文件
    char ip[31];                // 服务端IP地址
    int  port;                  // 服务端的端口号
    int  ptype;                 // 文件上传成功后文件的处理方式：1-删除文件；2-移动到备份目录
    char clientpath[301];       // 客户端文件存放的目录
    char clientpathbak[301];    // 客户端文件备份的目录，仅在ptype==2时生效
    bool andchild;              // 是否上传clientpath目录下的各级子目录的文件
    char matchname[301];        // 待上传文件名的匹配规则。如：*.TXT,&.XML
    char serverpath[301];       // 服务端文件的存放目录
    int  timetvl;               // 扫描本地目录文件的时间间隔，单位：秒
    int  timeout;               // 进程心跳的超时时间
    char pname[51];             // 进程名，建议采用"tcpputfiles_后缀"的方式
} starg;

CTcpClient TcpClient;   // TCP类
CLogFile   logfile;     // 日志类
CPActive   PActive;     // 心跳类

char strrecvbuffer[1024];   // 存储接受服务端发来的信息
char strsendbuffer[1024];   // 存储发送到服务端的信息

//////////////////main////////////////////
// 程序的帮助文档
void _help();
// 获取xmlbuffer的信息到starg结构体中
bool _xmltoarg(const char *xmlbuffer);
// 登录，与服务端确认连接参数
bool _login(const char *argv);
// 上传文件主函数，执行一次文件上传的任务。
bool _tcpputfiles();
// 保持心跳
bool ActiveTest();
// 进程退出
void EXIT(int sig);
//////////////////////////////////////////

//////////////////子函数///////////////////
// 上传文件内容的函数。执行在上传文件主函数(_tcpputfile)内部
bool SendFile(const int sockfd, const char *filename, const int filesize);
// 删除或者转存本地的文件
bool AckMessage(const char *buffer);
//////////////////////////////////////////

int main(int argc, char *argv[])
{
    // 生成程序帮助文档
    if (argc != 3) { _help(); return -1; }

    CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    if (logfile.Open(argv[1], "w+") == false)
    { printf("main:\tlogfile.Open(%s) failed!\n", argv[1]); return -1; }
    
    // 获取xml信息到starg
    if (_xmltoarg(argv[2]) == false)
    { logfile.Write("main:\t_xmltoatg() failed!\n"); EXIT(-1); }

    if ( TcpClient.ConnectToServer(starg.ip, starg.port) == false )
    { printf("The client failed to try to connect to the server(%s:%d)!\n",starg.ip, starg.port); EXIT(-1); }
    
    // 登录业务
    if ( _login(argv[2]) == false ) { logfile.Write("main:\t_login() failed!\n"); return -1; }
    
    while (true)
    {
        // 调用上传文件主函数，执行一次文件上传的任务
        if (_tcpputfiles() == false) { logfile.Write("_tcpputfiles() failed.\n"); EXIT(-1); }

        if ( ActiveTest() == false ) break;
        
        sleep(starg.timetvl);
    }

    EXIT(0);
}

// 程序的帮助文档
void _help()
{
    printf("Using:tcpputfiles logfile xmlbuffer\n");
    printf("Example:/project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/tools/tcpputfiles.log \"<ip>10.0.8.4</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcp/surfdata_client</clientpath><clientpathbak>/tmp/tcp/surfdata_clientbak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV</matchname><serverpath>/tmp/tcp/surfdata_server</serverpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n\n");
    printf("        /project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/tools/tcpputfiles.log \"<ip>10.0.8.4</ip><port>5005</port><ptype>2</ptype><clientpath>/tmp/tcp/surfdata_client</clientpath><clientpathbak>/tmp/tcp/surfdata_clientbak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV</matchname><serverpath>/tmp/tcp/surfdata_server</serverpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n\n");

    printf("logfile       日志文件存放地点\n");
    printf("xmlbuffer     客户端与服务端进行上传操作，用户需要用到的参数\n");
    // printf("  clienttype            客户端类型。1-上传文件；2-下载文件\n");
    printf("  ip                    服务端IP地址\n");
    printf("  port                  服务端的端口号\n");
    printf("  ptype                 文件上传成功后文件的处理方式：1-删除文件；2-移动到备份目录\n");
    printf("  clientpath            客户端文件存放的目录\n");
    printf("  clientpathbak         客户端文件备份的目录，仅在ptype==2时生效\n");
    printf("  andchild              是否上传clientpath目录下的各级子目录的文件\n");
    printf("  matchname             待上传文件名的匹配规则。如：*.TXT,&.XML\n");
    printf("  serverpath            服务端文件的存放目录\n");
    printf("  timetvl               扫描本地目录文件的时间间隔，单位：秒\n");
    printf("  timeout               进程心跳的超时时间\n");
    printf("  pname                 进程名，建议采用\"tcpputfiles_后缀\"的方式\n");
}

// 获取xmlbuffer的信息到starg结构体中
bool _xmltoarg(const char *xmlbuffer)
{
    memset(&starg, 0, sizeof(struct st_arg));

    // GetXMLBuffer(xmlbuffer, "clienttype", &starg.clienttype);               // 客户端类型。1-上传文件；2-下载文件
    // if ( (starg.clienttype != 1) && (starg.clienttype != 2) )
    // { logfile.Write("clienttype的值不合法!\n"); return false; }
    
    GetXMLBuffer(xmlbuffer, "ip", starg.ip, 30);                            // 服务端IP地址
    if (strlen(starg.ip) == 0)
    { logfile.Write("ip is null\n"); return false; }

    GetXMLBuffer(xmlbuffer, "port", &starg.port);                           // 服务端的端口号
    if (starg.port == 0)
    { logfile.Write("port is null\n"); return false; }

    GetXMLBuffer(xmlbuffer, "ptype", &starg.ptype);                         // 文件上传成功后文件的处理方式：1-删除文件；2-移动到备份目录
    if ((starg.ptype != 1) && (starg.ptype != 2))
    { logfile.Write("ptype is not (1,2)\n"); return false; }

    GetXMLBuffer(xmlbuffer, "clientpath", starg.clientpath, 300);           // 客户端文件存放的目录
    if (strlen(starg.clientpath) == 0)
    { logfile.Write("clientpath is null\n"); return false; }

    GetXMLBuffer(xmlbuffer, "clientpathbak", starg.clientpathbak, 300);     // 客户端文件备份的目录，仅在ptype==2时生效
    if (strlen(starg.clientpathbak) == 0)
    { logfile.Write("clientpathbak is null\n"); return false; }

    GetXMLBuffer(xmlbuffer, "andchild", &starg.andchild);                   // 是否上传clientpath目录下的各级子目录的文件
    // if (starg. != false) starg.andchild = true;

    GetXMLBuffer(xmlbuffer, "matchname", starg.matchname, 300);             // 待上传文件名的匹配规则。如：*.TXT,&.XML
    if (strlen(starg.matchname) == 0)
    { logfile.Write("matchname is null\n"); return false; }

    GetXMLBuffer(xmlbuffer, "serverpath", starg.serverpath, 300);           // 服务端文件的存放目录
    if (strlen(starg.serverpath) == 0)
    { logfile.Write("serverpath is null\n"); return false; }

    GetXMLBuffer(xmlbuffer, "timetvl", &starg.timetvl);                     // 扫描本地目录文件的时间间隔，单位：秒
    if (starg.timetvl == 0)
    { logfile.Write("timetvl is null\n"); return false; }
    if (starg.timetvl > 30) starg.timetvl = 30; // timetvl没有必要大于30

    GetXMLBuffer(xmlbuffer, "timeout", &starg.timeout);                     // 进程心跳的超时时间
    if (starg.timeout == 0)
    { logfile.Write("timeout is null\n"); return false; }
    if (starg.timeout < 50) starg.timeout = 50; // timeout没有必要小于50

    GetXMLBuffer(xmlbuffer, "pname", starg.pname, 50);                      // 进程名，建议采用\"tcpputfiles_后缀\"的方式
    if (strlen(starg.pname) == 0)
    { logfile.Write("pname is null\n"); return false; }

    return true;
}

// 登录
bool _login(const char *argv)
{
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

    SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1024, "<clienttype>1</clienttype>%s", argv);
    logfile.Write("发送:%s\n", strsendbuffer);
    if ( TcpClient.Write(strsendbuffer) == false ) return false;  // 向服务端发送请求报文

    if ( TcpClient.Read(strrecvbuffer, 20) == false ) return false;
    if (strcmp(strrecvbuffer, "failed") == 0)
    { logfile.Write("_login---(接收):%s\n", strrecvbuffer); return false; }
    logfile.Write("_login---(接收):%s\n", strrecvbuffer);

    logfile.Write("_login---:登录服务端(%s:%d)成功！\n", starg.ip, starg.port);

    return true;
}

// 上传文件主函数，执行一次文件上传的任务。
bool _tcpputfiles()
{
    CDir Dir;
    // 调用OpenDir()打开starg.clientpath目录
    if (Dir.OpenDir(starg.clientpath, starg.matchname, 10000, starg.andchild) == false)
    { logfile.Write("Dir.OpenDir(%s, %s) failed.\n", starg.clientpath, starg.matchname); return false; }

    while (true)
    {
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
        // 遍历目录中的每个文件，调用ReadDir()获取一个文件名
        if (Dir.ReadDir() == false) break;

        // 把文件名、修改时间、文件大小组成报文，发送给对端
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1024, "<filename>%s</filename><mtime>%s</mtime><msize>%d</msize>", Dir.m_FullFileName, Dir.m_ModifyTime, Dir.m_FileSize);
        if (TcpClient.Write(strsendbuffer) == false)
        { logfile.Write("_tcpputfiles---(文件信息发送): TcpClient.Write(%s) falied.\n", Dir.m_FullFileName); return false; }
        // logfile.Write("_tcpputfiles---(文件信息发送): %s\n", strsendbuffer);

        // 把文件的内容发送给对端
        // logfile.Write("_tcpputfiles---(文件内容发送): %s(%d) ... ", Dir.m_FullFileName, Dir.m_FileSize);
        logfile.Write("发送：%s(%d) ... ", Dir.m_FullFileName, Dir.m_FileSize);
        if (SendFile(TcpClient.m_connfd, Dir.m_FullFileName, Dir.m_FileSize) == false)
        { logfile.WriteEx("failed.\n"); TcpClient.Close(); return false; }
        logfile.WriteEx("successed.\n");

        // 接收对端的确认报文
        if (TcpClient.Read(strrecvbuffer, 20) == false)
        { logfile.Write("TcpClient.Read(%s) failed.\n", strrecvbuffer); return false; }
        // logfile.Write("_tcpputfiles---(接收): %s\n", strrecvbuffer);

        // 删除或者转存本地的文件
        AckMessage(strrecvbuffer);
    }

    return true;
}

// 发送心跳报文
bool ActiveTest()
{
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<activetest>ok</activetest>");
    // logfile.Write("ActiveTest---(发送): %s\n", strsendbuffer);
    if ( TcpClient.Write(strsendbuffer) == false ) return false;  // 向服务端发送请求报文

    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if ( TcpClient.Read(strrecvbuffer, 20) == false ) return false;
    logfile.Write("ActiveTest---(接收): %s\n", strrecvbuffer);

    return true;
}

// 进程退出
void EXIT(int sig)
{
    logfile.Write("接收到%d信号，进程(%d)退出", sig, getpid());
    exit(0);
}

// 上传文件内容的函数。执行在上传文件主函数(_tcpputfile)内部
bool SendFile(const int sockfd, const char *filename, const int filesize)
{
    int  onread = 0;        // 每次调用fread时打算读取的字节数
    int  bytes  = 0;        // 调用一次fread从文件中读取的字节数
    char buffer[1001];      // 存放读取数据的buffer
    int  totalbytes = 0;    // 从文件中以读取的字节总数
    FILE *fp = NULL;

    // 以"rb"的模式打开文件
    if ( (fp = fopen(filename, "rb")) == NULL )
    { logfile.Write("SendFile---:fopen(%s) failed.\n", filename); return false; }

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        // 计算本次应该读取的字节数，如果剩余的数据超过1000字节，就打算读1000字节
        if (filesize-totalbytes > 1000) onread = 1000;
        else onread = filesize-totalbytes; 
        
        // 从文件中读取数据
        bytes = fread(buffer, 1, onread, fp);
        
        // 把读取到的数据发送给服务端
        if (bytes > 0)
        {
            if (Writen(sockfd, buffer, bytes) == false) { fclose(fp); return false; }
        }

        // 计算文件以读取的总字节数，如果文件已经读取完，跳出循环
        totalbytes = totalbytes+bytes;
        
        if (totalbytes == filesize) break;
    }
    
    fclose(fp);
    
    return true;
}

// 删除或者转存本地的文件
bool AckMessage(const char *buffer)
{
    char filename[301];     memset(filename, 0, sizeof(filename));
    char result[11];        memset(result, 0, sizeof(result));
    
    GetXMLBuffer(buffer, "filename", filename, 300);
    GetXMLBuffer(buffer, "result", result, 10);
    
    // 如果服务端没有返回successed，直接返回
    if (strcmp(result, "successed") != 0) return false;
    
    // ptype == 1，删除文件     REMOVE
    if (starg.ptype == 1)
    {
        if (REMOVE(filename) == false) { logfile.Write("REMOVE(%s) failed.\n", filename); }
    }

    char filenamebak[301];
    if (starg.ptype == 2)
    {
        // 先生成备份目录的文件名
        STRCPY(filenamebak, 300, filename);
        UpdateStr(filenamebak, starg.clientpath, starg.clientpathbak, false);
        if (RENAME(filename, filenamebak) == false) { logfile.Write("RENAME(%s, %s) failed.\n", filename, filenamebak); }
    }

    return true;
}