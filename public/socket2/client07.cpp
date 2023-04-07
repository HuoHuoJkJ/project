#include "../_public.h"

CTcpClient TcpClient;
CLogFile logfile;

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
    
    char buffer[1024];
    for (int ii = 0; ii < 5; ii++)
    {
        SPRINTF(buffer, sizeof(buffer), "hahajiukanjian 发送 %010d", ii+1);
        if (TcpClient.Write(buffer) == false) break;
        logfile.Write("发送:%s\n", buffer);

        memset(buffer, 0, sizeof(buffer));
        if (TcpClient.Read(buffer) == false) break;
        logfile.Write("接收:%s\n", buffer);
        sleep(1);
    }

    return 0;
}