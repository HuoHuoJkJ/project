/*
 * 创建客户端socket
 * 向服务器发起连接请求
 * 与服务器通信，发送一个报文后等待回复，然后再发下一个报文
 * 不断重复上一步，直到全部的数据被发送完
 * 关闭socket，释放资源
 */
#include "../_public.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Using:./client30 ip port\nExample:./client30 127.0.0.1 8080\n\n"); return -1; }

    CTcpClient TcpClient;

    if ( TcpClient.ConnectToServer(argv[1], atoi(argv[2])) == false )
    { printf("客户端尝试连接服务端(%s)失败！\n", argv[1]); return -1; }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    sprintf(buffer, \
    "GET / HTTP/1.1\r\n"\
    "Host: %s:%s\r\n"\
    "\r\n", argv[1], argv[2]);

    send(TcpClient.m_connfd, buffer, 1000, 0);

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        if (recv(TcpClient.m_connfd, buffer, 1000, 0) <= 0) return 0;
        printf("%s\n", buffer);
    }

    return 0;
}