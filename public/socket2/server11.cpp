/* 
 * server11.cpp 作为服务端，接受客户端的数据，用来演示同步通信的效率
 */
#include "../_public.h"

CLogFile    logfile;        // 服务器日志文件
CTcpServer  TcpServer;      // 创建服务对象

bool logstatus = false;

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Use:./server11 port logfile\nExample:./server11 5005 /tmp/server11.log\n"); return -1; }
 
    if ( logfile.Open(argv[2], "w+") == false ) { printf("打开日志文件失败！\n"); return -1; }

    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { logfile.Write("服务器设置端口失败！\n"); return -1; }
    
    if ( TcpServer.Accept() == false )
    {
        printf("Accept() failed!\n"); return -1;
    }

    char strrecvbuffer[1024], strsendbuffer[1024];
    while (1)
    {
        memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
        memset(strsendbuffer, 0, sizeof(strsendbuffer));

        if ( TcpServer.Read(strrecvbuffer) == false ) break;
        logfile.Write("接受：%s\n", strrecvbuffer);

        // SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1024, "ok");
        // if ( TcpServer.Write(strsendbuffer) == false ) break;
        // logfile.Write("发送：%s\n", strsendbuffer);
    }

    return 0;
}
