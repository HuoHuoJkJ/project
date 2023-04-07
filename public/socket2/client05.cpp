#include "../_public.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Using:./client ip port\nExample:./client 127.0.0.1 5005\n\n");
        return -1;
    }
    CTcpClient TcpClient;
    if (TcpClient.ConnectToServer(argv[1], atoi(argv[2])) == false)
    {
        printf("ConnectToServer(%s) 失败\n", argv[1]);
        return -1;
    }
    
    char buffer[1024];
    for (int ii = 0; ii < 1000; ii++)
    {
        SPRINTF(buffer, sizeof(buffer), "hahajiukanjian 发送 %010d", ii+1);
        if (TcpClient.Write(buffer) == false) break;
        printf("发送:%s\n", buffer);

        memset(buffer, 0, sizeof(buffer));
        if (TcpClient.Read(buffer) == false) break;
        printf("接收:%s\n", buffer);
    }


    return 0;
}