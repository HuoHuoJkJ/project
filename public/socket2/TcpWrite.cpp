#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

bool TcpWrite(int sockfd, const char *buffer, const int ibuflen);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Using:./TcpWrite ip port\nExample:./client 127.0.0.1 5005\n\n");
        return -1;
    }

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
    sprintf(buffer, "hahajiukanjian\n");

    TcpWrite(sockfd, buffer, sizeof(buffer));
    printf("发送：%s\n", buffer);

    return 0;
}

bool TcpWrite(int sockfd, const char *buffer, const int ibuflen)
{
    // 判断是否存在sockfd
    if (sockfd == -1) return false;
    
    int ilen = 0;
    // 如果传入的是ascii字符流
    if (ibuflen == 0)
        ilen = strlen(buffer);
    else
        ilen = ibuflen;
    
    int ilenn = htonl(ilen);
    char TBuffer[ilen+4];
    memset(TBuffer, 0, sizeof(TBuffer));
    memcpy(TBuffer, &ilenn, 4);
    memcpy(TBuffer+4, buffer, ilen);
    
    int nLeft = ilen+4, idx = 0, nwritten;
    while (nLeft > 0)
    {
        if ((nwritten = send(sockfd, TBuffer+idx, nLeft, 0)) <= 0)
        idx = idx + nwritten;
        nLeft = nLeft - nwritten;
    }

    return true;
}