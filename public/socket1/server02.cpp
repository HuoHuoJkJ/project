/*
 * demo02.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
 * socket
 * sockaddr_in
 * bind
 * listen
 * accept
 * recv
 * send
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    { printf("Use:./server port\nExample:./server 5005\n"); return -1; }

    // 创建socket，返回一个文件描述符
    int listenfd;
    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == 0 )
    { printf("创建socket失败\n"); return -1; }

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
/*
    while (1)
    {
        if ( (clienfd = accept(listenfd, (struct sockaddr *)&clientaddr, (socklen_t *)&socklen)) <= 0 ) continue;
        printf("客户端(%s)已连接。sockfd = %d\n", inet_ntoa(clientaddr.sin_addr), clienfd);
        sleep(10);
    }
*/
    int iret;
    char buffer[59];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if ( (iret = recv(clienfd, buffer, 10, 0)) <= 0 )
        {
            printf("iret = %d\n", iret);
            break;
        }
        // printf("iret=%d,接受：%s\n", iret, buffer);
        printf("iret = %d; %s\n", iret, buffer);

        usleep(100);

        // strcpy(buffer, "ok");
        // if ( (iret = send(clienfd, buffer, strlen(buffer), 0)) <= 0 )
        // {
        //     perror("send");
        //     break;
        // }
        // printf("发送：%s\n", buffer);
    }

    close(listenfd);
    close(clienfd);
    return 0;
}