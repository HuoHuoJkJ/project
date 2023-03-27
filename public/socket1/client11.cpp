/*
 * 创建客户端socket
 * 向服务器发起连接请求
 * 与服务器通信，发送一个报文后等待回复，然后再发下一个报文
 * 不断重复上一步，直到全部的数据被发送完
 * 关闭socket，释放资源
 */
#include "../_public.h"

CTcpClient TcpClient;

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Using:./client11 ip port\nExample:./client11 10.0.8.4 5005\n\n"); return -1; }
    
    if ( TcpClient.ConnectToServer(argv[1], atoi(argv[2])) == false )
    { printf("客户端尝试连接服务端(%s)失败！\n", argv[1]); return -1; }
    
    char strrecvbuffer[1024], strsendbuffer[1024];

    for (int ii = 0; ii < 100000; ii++)
    {
        memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1024, "你好，这是发送的第%d条数据", ii+1);
        TcpClient.Write(strsendbuffer);
        TcpClient.Read(strrecvbuffer);
    }
    
    return 0;
}