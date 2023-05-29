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
    { printf("Using:./client06 ip port\nExample:./client01 127.0.0.1 5000\n\n"); return -1; }

    CTcpClient TcpClient;

    if ( TcpClient.ConnectToServer(argv[1], atoi(argv[2])) == false )
    { printf("客户端尝试连接服务端(%s)失败！\n", argv[1]); return -1; }

    char buffer[1024];
    // 与服务端通信，发送一个报文后等待回复，然后再发下一个报文
    for (int ii = 0; ii < 20; ii++)
    {
        SPRINTF(buffer, sizeof(buffer), "这是发送的第%010d个数据，编号为%010d", ii + 1, ii + 1);
        // 向服务端发送请求报文
        if ( TcpClient.Write(buffer) == false ) break;
        // printf("发送:%s\n", buffer);

        memset(buffer, 0, sizeof(buffer));
        if ( TcpClient.Read(buffer) == false ) break;
break;
        // printf("接受%s\n", buffer);
        sleep(2);
    }

    return 0;
}