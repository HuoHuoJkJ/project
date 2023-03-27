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

// 程序的帮助文档
void _help();
// 获取xmlbuffer的信息到starg结构体中
bool _xmltoarg(const char *xmlbuffer);

bool ActiveTest();      // 保持心跳
bool Login(const char *argv);      // 登录业务

// 进程退出
void EXIT(int sig);

int main(int argc, char *argv[])
{
    // 生成程序帮助文档
    if (argc != 3) { _help(); return -1; }

    CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    if (logfile.Open(argv[1], "a+") == false)
    { printf("main:\tlogfile.Open(%s) failed!\n", argv[1]); return -1; }
    
    // 获取xml信息到starg
    if (_xmltoarg(argv[2]) == false)
    { logfile.Write("main:\t_xmltoatg() failed!\n"); EXIT(-1); }

    if ( TcpClient.ConnectToServer(starg.ip, starg.port) == false )
    { printf("The client failed to try to connect to the server(%s:%d)!\n",starg.ip, starg.port); EXIT(-1); }
    
    // 登录业务
    if ( Login(argv[2]) == false ) { logfile.Write("main:\tLogin failed!\n"); return -1; }

    for (int ii = 3; ii < 5; ii++)
    {
        if ( ActiveTest() == false ) EXIT(-1);
        sleep(ii);
    }

    EXIT(0);
}

// 程序的帮助文档
void _help()
{
    printf("Using:tcpputfiles logfile xmlbuffer\n");
    printf("Example:/project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/tools/tcpputfiles.log \"<ip>10.0.8.4</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcp/surfdata</clientpath><clientpathbak>/tmp/tcp/surfdatabak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV</matchname><serverpath>/tmp/tcp/surfdata_server</serverpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n\n");
    printf("        /project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/tools/tcpputfiles.log \"<ip>10.0.8.4</ip><port>5005</port><ptype>2</ptype><clientpath>/tmp/tcp/surfdata</clientpath><clientpathbak>/tmp/tcp/surfdatabak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV</matchname><serverpath>/tmp/tcp/surfdata_server</serverpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n\n");

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

// 发送心跳报文
bool ActiveTest()
{
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<activetest>ok</activetest>");
    logfile.Write("发送:%s\n", strsendbuffer);
    if ( TcpClient.Write(strsendbuffer) == false ) return false;  // 向服务端发送请求报文

    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if ( TcpClient.Read(strrecvbuffer, 20) == false ) return false;
    logfile.Write("接受%s\n", strrecvbuffer);

    return true;
}

// 登录
bool Login(const char *argv)
{
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

    SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 300, "<clienttype>1</clienttype>%s", argv);
    logfile.Write("发送:%s\n", strsendbuffer);
    if ( TcpClient.Write(strsendbuffer) == false ) return false;  // 向服务端发送请求报文

    if ( TcpClient.Read(strrecvbuffer, 20) == false ) return false;
    logfile.Write("接受%s\n", strrecvbuffer);
    
    logfile.Write("登录服务端(%s:%d)成功！\n", starg.ip, starg.port);

    return true;
}

// 进程退出
void EXIT(int sig)
{
    logfile.Write("接收到%d信号，进程(%d)退出", sig, getpid());
    exit(0);
}