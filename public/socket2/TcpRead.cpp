#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

bool TcpRead(int sockfd, const char *buffer, int *ibuflen);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("./TcpRead 5005\n");
        return -1;
    }
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }
    // 创建一个sockaddr_inj结构体，用来存储端口信息和ip地址信息等。将其作为参数传入bind函数中，与socket进行绑定
    struct sockaddr_in servaddr; // 服务端地址信息的数据结构。
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family         = AF_INET;
    servaddr.sin_port           = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr    = htonl(INADDR_ANY);
    if ( bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0 )
    {
        perror("bind");
        close(listenfd);
        return -1;
    }

    // 服务端在这里进行监听
    if ( listen(listenfd, 3) != 0 )
    {
        perror("listen");
        close(listenfd);
        return -1;
    }

    // 获取客户端的连接数据
    int clienfd; // 创建一个处理客户端的文件描述符
    int socklen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    clienfd = accept(listenfd, (struct sockaddr *)&clientaddr, (socklen_t *)&socklen);
    printf("客户端(%s)已连接。sockfd = %d\n", inet_ntoa(clientaddr.sin_addr), clienfd);

    char buffer[1024];
    int ibuflen;
    TcpRead(clienfd, buffer, &ibuflen);
    printf("接收:%s\n", buffer);

    return 0;
}

bool TcpRead(int sockfd, const char *buffer, int *ibuflen)
{
    (*ibuflen) = 0;
    
    int nLeft = 4, idx = 0, nread;
    while (nLeft > 0)
    {
        if ((nread = recv(sockfd, (char *)ibuflen+idx, nLeft, 0)) <= 0)
        return false;
        
        idx = idx + nread;
        nLeft = nLeft - nread;
    }

    (*ibuflen) = ntohl(*ibuflen);

    nLeft = *ibuflen, idx = 0;
    while (nLeft > 0)
    {
        if ((nread = recv(sockfd, (char *)buffer+idx, nLeft, 0)) <= 0)
        return false;
        
        idx = idx + nread;
        nLeft = nLeft - nread;
    }

    return true;
}