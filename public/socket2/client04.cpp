#include "../_public.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Using:./client ip port\nExample:./client 127.0.0.1 5005\n\n");
        return -1;
    }
    // 第一步：创建客户端的socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }
    // 第二步：向服务器发起连接请求
    struct hostent *h;
    if ((h = gethostbyname(argv[1])) == 0) // 指定服务端的ip地址
    {
        printf("gethostbyname failed.\n");
        close(sockfd);
        return -1;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2])); // 指定服务端的通信端口
    memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);
    // 向服务端发起连接请求
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        perror("connect");
        close(sockfd);
        return -1;
    }

    char buffer[1024];

    // 第三步：与服务端通信，发送一个报文后等待回复，然后再发下一个报文
    for (int ii = 0; ii < 50; ii++)
    {
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "这是发送的第%010d个数据，编号为%010d", ii + 1, ii + 1);
        // 向服务端发送请求报文
        if (TcpWrite(sockfd, buffer, sizeof(buffer)) == false) break;

        printf("发送:%s\n", buffer);
    }

    // 第四步：关闭socket，释放资源
    close(sockfd);

    return 0;
}