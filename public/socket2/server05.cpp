#include "../_public.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    { printf("Use:./server port\nExample:./server 5005\n"); return -1; }
    
    CTcpServer TcpServer;
    TcpServer.InitServer(atoi(argv[1]));
    TcpServer.GetIP();
    while (true)
    {
        if (TcpServer.Accept() == true) break;
    }
    
    char buffer[1024];
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        if (TcpServer.Read(buffer) == false) break;
        printf("接收:%s\n", buffer);

        STRCPY(buffer, sizeof(buffer), "已接收");
        if (TcpServer.Write(buffer) == false) break;
        printf("发送:%s\n", buffer);
    }


    return 0;
}