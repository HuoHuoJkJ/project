#include "../_public.h"

CTcpClient TcpClient;
CLogFile logfile;

char strsendbuffer[1024];
char strrecvbuffer[1024];

bool srv000();      // 心跳报文
bool srv001();      // 登录
bool srv002();      // 查询余额

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Using:./client ip port logfile\nExample:./client01 127.0.0.1 5005 /project/public/socket2/client01.log\n\n");
        return -1;
    }

    // 打开日志文件
    if (logfile.Open(argv[3], "a+") == false)
    { printf("日志文件(%s)失败\n", argv[3]); return -1; }

    // 创建tcp连接
    if (TcpClient.ConnectToServer(argv[1], atoi(argv[2])) == false)
    {
        logfile.Write("ConnectToServer(%s) 失败\n", argv[1]);
        return -1;
    }
    
    srv000();
    srv001();
    sleep(1);
    srv002();
    
    return 0;
}

bool srv000()
{
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<srvcode>0<srvcode><message>hahajiukanjian</message>");
    if (TcpClient.Write(strsendbuffer) == false)
    {
        logfile.Write("发送心跳报文: %s 失败\n", strsendbuffer);
        return false;
    }
    logfile.Write("发送心跳报文: %s\n", strsendbuffer);

    if (TcpClient.Read(strrecvbuffer) == false)
    {
        logfile.Write("接收心跳报文: %s 失败\n", strrecvbuffer);
        return false;
    }
    logfile.Write("接收心跳报文: %s\n", strrecvbuffer);

    return true;
}

// 登录
bool srv001()
{
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<srvcode>1</srvcode><username>hahajiukanjian</username><password>dyt123</password>");
    if (TcpClient.Write(strsendbuffer) == false)
    {
        logfile.Write("发送登录数据(%s)失败\n", strsendbuffer);
        return false;
    }
    logfile.Write("发送登录数据:%s\n", strsendbuffer);

    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if (TcpClient.Read(strrecvbuffer) == false)
    {
        logfile.Write("获取服务端发送的登录处理结果失败\n");
        return false;
    }
    logfile.Write("接收登录数据:%s\n", strrecvbuffer);

    int iretcode = -1;
    GetXMLBuffer(strrecvbuffer, "retcode", &iretcode);
    if (iretcode != 0)
    {
        logfile.Write("登录失败，用户名或密码不正确\n");
        return false;
    }
    logfile.Write("登录成功\n");

    return true;
}

// 查询余额
bool srv002()
{
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<srvcode>2</srvcode><cardid>1695259395</cardid>");
    if (TcpClient.Write(strsendbuffer) == false)
    {
        logfile.Write("发送卡号信息(%s)失败\n", strsendbuffer);
        return false;
    }
    logfile.Write("发送卡号信息:%s\n", strsendbuffer);

    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if (TcpClient.Read(strrecvbuffer) == false)
    {
        logfile.Write("获取服务端发送的查询余额结果失败\n");
        return false;
    }
    logfile.Write("接收余额信息:%s\n", strrecvbuffer);
    int iretcode = -1;
    GetXMLBuffer(strrecvbuffer, "retcode", &iretcode);
    if (iretcode != 0)
    {
        logfile.Write("查询余额失败\n");
        return false;
    }
    double ye;
    GetXMLBuffer(strrecvbuffer, "ye", &ye);
    logfile.Write("查询成功，余额：%.2f\n", ye);

    return true;
}