/*
 * server.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
 */
#include "../_public.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    { printf("Use:./server30 port\nExample:./server30 5000\n"); return -1; }

    CTcpServer TcpServer;

    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { printf("服务器设置端口失败！\n"); return -1; }

    if ( TcpServer.Accept() == false )
    { printf("Accept() failed!\n"); return -1; }
    printf("客户端(%s)连接成功！\n", TcpServer.GetIP());

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    recv(TcpServer.m_connfd, buffer, 1000, 0);
    printf("%s\n", buffer);

    return 0;
}