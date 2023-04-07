#include "_public.h"

/* 
 * 登录确定连接信息
 * 发送文件信息
 * 发送文件内容
 * 对本地文件进行处理
 */
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
CTcpClient  TcpClient;
CPActive    PActive;

char sendbuffer[1024];
char recvbuffer[1024];


void _help();
bool _xmltoarg(const char *buffer);
bool _sendxml(const char *argv);
bool _active();
bool _sendfilesmain();
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

    // 连接tcp服务器
    TcpClient.ConnectToServer(starg.ip, starg.port);
    
    // 向服务端确认连接参数
    if (_sendxml(argv[2]) == false) { logfile.Write("向服务端传输参数失败\n"); _exit(-1); }
    
    // 发送心跳报文
    if (_active() == false) { logfile.Write("心跳报文发送失败\n"); _exit(-1); }

    logfile.Write("\n\n\n\n\n\n");

    return 0;
}

void _help()
{
    printf("Use:tcpputfiles logfile xmlbuffer\n");    
    printf("Example:/project/tools1/bin/tcpputfiles /log/tools/tcpputfiles1.log \"<ip>43.143.136.101</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcp/client</clientpath><serverpath>/tmp/tcp/server</serverpath><clientpathbak>/tmp/tcp/clientbak</clientpathbak><matchname>*.TXT</matchname><andchild>true</andchild><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles</pname>\"\n");    
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

    GetXMLBuffer(buffer, "clientpathbak", starg.clientpathbak, 300);
    if (starg.ptype == 2 && strlen(starg.clientpathbak) == 0) { logfile.Write("clientpathbak的值为空\n"); return false; }

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
    
    SPRINTF(sendbuffer, sizeof(sendbuffer), "<clienttype>1</clienttype>%s", argv);
    if (TcpClient.Write(sendbuffer) == false) return false;
    logfile.Write("发送--确认参数：%s\n", sendbuffer);

    if (TcpClient.Read(recvbuffer, 20) == false) return false;
    logfile.Write("接收--回应参数：%s\n", recvbuffer);

    logfile.Write("信息--连接成功：%s:%d\n", starg.ip, starg.port);

    return true;
}

bool _active()
{
    memset(sendbuffer, 0, sizeof(sendbuffer));
    memset(recvbuffer, 0, sizeof(recvbuffer));

    SPRINTF(sendbuffer, sizeof(sendbuffer), "<activetest>hahajiukanjian</activetest>");
    if (TcpClient.Write(sendbuffer) == false) return false;
    logfile.Write("发送--心跳报文：%s\n", sendbuffer);

    if (TcpClient.Read(recvbuffer, 20) == false) return false;
    logfile.Write("接收--回应报文：%s\n", recvbuffer);

    return true;
}

bool _sendfilesmain()
{
    CDir Dir;
    // 打开clientpath目录
    if (Dir.OpenDir(starg.clientpath, starg.matchname, 10000, starg.andchild) == false)
    { logfile.Write("打开目录(%s)失败\n"); return false; }
    
    // 遍历目录下的文件内容
    while (true)
    {
        // 获取一个文件信息
        if (Dir.ReadDir() == false) break;
        char filename[301];
        char ptime[21];
        int  psize = 0;

        STRCPY(filename, sizeof(filename), Dir.m_FullFileName);
        STRCPY(ptime, sizeof(ptime), Dir.m_ModifyTime);
        psize = Dir.m_FileSize;
        SPRINTF(sendbuffer, sizeof(sendbuffer), "<filename>%s</filename><ptime>%s</ptime><psize>%d</psize>", filename, ptime, psize);

        // 将文件大小，时间，文件名发送到服务端
        if (TcpClient.Write(sendbuffer) == false) return false;
        logfile.Write("发送--文件信息：%s\n", sendbuffer);
        
        if (TcpClient.Read(recvbuffer, 20) == false) return false;
        logfile.Write("接收--信息回应：%s\n", recvbuffer);
    }

    return true;
}

void _exit(int sig)
{
    logfile.Write("程序即将推出，sig=%d\n", sig);

    exit(0);
}