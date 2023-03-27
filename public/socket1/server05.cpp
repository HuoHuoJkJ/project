/* 
 * server.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
 */
#include "../_public.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    { printf("Use:./server port\nExample:./server 5005\n"); return -1; }
    
    CTcpServer TcpServer;
    
    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { printf("服务器设置端口失败！\n"); return -1; }

    TcpServer.Accept();

    char buffer[1024];
    while (1)
    {
        if ( TcpServer.Read(buffer) == false ) break;
        printf("接受：%s\n", buffer);

        usleep(10000);

        SPRINTF(buffer, sizeof(buffer), "ok");
        if ( TcpServer.Write(buffer) == false ) break;
        printf("发送：%s\n", buffer);
    }
    
    return 0;
}