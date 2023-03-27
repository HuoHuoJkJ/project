/* 
 * server01.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
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

char strrecvbuffer[1024];   // 存储接受服务端发来的信息
char strsendbuffer[1024];   // 存储发送到服务端的信息

CLogFile    logfile;        // 服务器日志文件
CTcpServer  TcpServer;      // 创建服务对象

// 获取xmlbuffer的信息到starg结构体中
bool _xmltoarg(const char *xmlbuffer);
// 登录
bool ClientLogin();
// 上传文件的主函数
void RecvFileMain();
// 父进程的退出信号
void FathEXIT(int sig);
// 子进程的退出信号
void ChldEXIT(int sig);

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Use:./fileserver port logfile\nExample:/project/tools1/bin/fileserver 5005 /log/tools/fileserver.log\n"); return -1; }
 
    // 关闭标准输入输出和信号，只保留信号2(SIGINT)和信号15(SIGTERM)
    CloseIOAndSignal(true); signal(SIGINT, FathEXIT); signal(SIGTERM, FathEXIT);

    if ( logfile.Open(argv[2], "a+") == false ) { printf("打开日志文件失败！\n"); return -1; }

    logfile.Write("pid=%d\n", getpid());

    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { logfile.Write("服务器设置端口失败！\n"); return -1; }
    
    while (true)
    {
        if ( TcpServer.Accept() == false )
        { printf("Accept() failed!\n"); return -1; }
        logfile.Write("客户端(%s)连接成功！\n", TcpServer.GetIP());
/* 
        if (fork() > 0) { TcpServer.CloseClient(); continue; }

        signal(SIGINT, ChldEXIT); signal(SIGTERM, ChldEXIT);

        TcpServer.CloseListen();
 */
        // 子进程与客户端进行通讯，处理业务
        
        // 处理登录客户端的登录报文
        if (ClientLogin() == false) ChldEXIT(-1); 

        // 如果clienttype==1，调用上传文件的主函数
        if (starg.clienttype == 1) RecvFileMain();

        // 如果clienttype==2，调用下载文件的主函数

        ChldEXIT(0);
    }

    return 0;
}

// 获取xmlbuffer的信息到starg结构体中
bool _xmltoarg(const char *xmlbuffer)
{
    memset(&starg, 0, sizeof(struct st_arg));

    GetXMLBuffer(xmlbuffer, "clienttype", &starg.clienttype);               // 客户端类型。1-上传文件；2-下载文件

    GetXMLBuffer(xmlbuffer, "ptype", &starg.ptype);                         // 文件上传成功后文件的处理方式：1-删除文件；2-移动到备份目录

    GetXMLBuffer(xmlbuffer, "clientpath", starg.clientpath, 300);           // 客户端文件存放的目录

    GetXMLBuffer(xmlbuffer, "andchild", &starg.andchild);                   // 是否上传clientpath目录下的各级子目录的文件

    GetXMLBuffer(xmlbuffer, "matchname", starg.matchname, 300);             // 待上传文件名的匹配规则。如：*.TXT,&.XML

    GetXMLBuffer(xmlbuffer, "serverpath", starg.serverpath, 300);           // 服务端文件的存放目录

    GetXMLBuffer(xmlbuffer, "timetvl", &starg.timetvl);                     // 扫描本地目录文件的时间间隔，单位：秒
    if (starg.timetvl > 30) starg.timetvl = 30; // timetvl没有必要大于30

    GetXMLBuffer(xmlbuffer, "timeout", &starg.timeout);                     // 进程心跳的超时时间
    if (starg.timeout < 50) starg.timeout = 50; // timeout没有必要小于50

    GetXMLBuffer(xmlbuffer, "pname", starg.pname, 50);                      // 进程名，建议采用\"tcpputfiles_后缀\"的方式
    strcat(starg.pname, "_server");

    return true;
}

// 登录
bool ClientLogin()
{
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    
    if (TcpServer.Read(strrecvbuffer, 20) == false)
    { logfile.Write("ClientLogin: TcpServer.Read(strrecvbuffer, 20) failed!\n"); return false; }
    logfile.Write("strrecvbuffer=%s\n", strrecvbuffer);

    _xmltoarg(strrecvbuffer);
    
    if ((starg.clienttype != 1) && (starg.clienttype != 2))
        strcpy(strsendbuffer, "failed");
    else
        strcpy(strsendbuffer, "ok");
    
    if (TcpServer.Write(strsendbuffer) == false)
    { logfile.Write("ClientLogin: TcpServer.Write(strsendbuffer) failed!\n"); return false; }
    logfile.Write("%s login %s\n",TcpServer.GetIP() , strsendbuffer);

    return true;
}

// 上传文件的主函数
void RecvFileMain()
{
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    
    while (true)
    {
        // 接收采用上传功能的客户端进程发送过来的内容
        if (TcpServer.Read(strrecvbuffer, starg.timetvl+10) == false)
        { logfile.Write("TcpServer.Read() failed!\n"); return; }
        logfile.Write("strrecvbuffer=%s\n", strrecvbuffer);
        
        // 判断客户端发送过来的是否为心跳报文
        if (strcmp(strrecvbuffer, "<activetest>ok</activetest>") == 0)
        {
            strcpy(strsendbuffer, "activetest ok");
            logfile.Write("strsendbuffer=%s\n", strsendbuffer);
            if (TcpServer.Write(strsendbuffer) == false)
            { logfile.Write("TcpServer.Write() failed!\n", strsendbuffer); return; }
        }
    }
}

// 父进程的退出信号
void FathEXIT(int sig)     
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    logfile.Write("父进程(%d)退出！sig=%d\n", getpid(), sig);

    TcpServer.CloseListen();        // 先关闭监听的socket

    kill(0, sig);                   // 通知全部的子进程退出

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