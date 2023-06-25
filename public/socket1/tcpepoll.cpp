/*
 * 使用select模型，处理多个客户端的连接请求
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

int initserver(int port);

int main(int argc, char *argv[])
{
    if (argc != 2)
    { printf("Use:./tcpselect 5010\n"); return -1; }

    /*
     * 初始化服务器
     */
    int listenfd;
    if ((listenfd = initserver(atoi(argv[1]))) < 0)
        printf("initserver(%s) failed!\n", argv[1]);
    printf("listenfd = %d\n", listenfd);

    // 创建epoll句柄
    int epollfd = epoll_create(1); // 参数在linux 2.X 已经被忽略了，只需要填大于0的数就行

    // printf("epollfd = %d\n", epollfd);

    // 为监听的sockfd准备可读事件
    struct epoll_event ev;  // 声明事件的数据结构
    ev.events = EPOLLIN;    // 读事件
    ev.data.fd = listenfd;  // 指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回

    // 把监听的sockfd的事件加入到epollfd中
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

    struct epoll_event evs[10];

    while (true)
    {
        // 等待监视的sockfd有事件发生
        int infds = epoll_wait(epollfd, evs, 10, -1);

        // 处理epoll_wait失败的情况
        if (infds < 0)
        {
            printf("epoll_wait() failed!\n");
            break;
        }

        // epoll_wait超时情况
        if (infds == 0)
        {
            printf("epoll_wait() timeout!\n");
            continue;
        }

        // 如果infds>0，表示有事件发生的socket的数量
        // 遍历epoll返回的已发生事件的数组
        for (int ii=0; ii<infds; ii++)
        {
            // printf("data.fd=%d, events=%d\n", evs[ii].data.fd, evs[ii].events);

            if (evs[ii].data.fd == listenfd)
            {
                // 进行accept的操作
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientfd = accept(listenfd, (struct sockaddr*)&client, &len);
                if (clientfd < 0)
                {
                    perror("accept");
                    continue;
                }
                printf("%ld accept client(sockfd=%d) ok.\n", time(0), clientfd);

                // 为新客户端准备可读事件，并添加到epoll中
                ev.data.fd = clientfd;
                ev.events = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
            }
            else
            {
                // 如果是客户端连接的sockfd事件，表示有报文发过来 或者 是连接已断开
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                if (recv(evs[ii].data.fd, buffer, sizeof(buffer), 0) <= 0)
                {
                    printf("%ld client(eventfd=%d) disconnected.\n", time(0), evs[ii].data.fd);
                    close(evs[ii].data.fd);
                }
                else
                {
                    // printf("recv(fd=%d):%s\n", evs[ii].data.fd, buffer);
                    send(evs[ii].data.fd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    return 0;
}

int initserver(int port)
{
    /*
     * 设置监听的socket套接字
     */
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("create socket failed!\n");
        return -1;
    }

    /*
     * 创建sockaddr_in结构体，用来存储端口信息和ip地址信息等。
     * 将其作为绑定参数传入bind函数中，与socket进行绑定
     */
    // 服务端地址信息的结构体
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    // 对servaddr结构体内的变量赋值
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    // 接受任意的地址的请求
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        perror("bind");
        close(listenfd);
        return -1;
    }

    /*
     * 监听请求
     * 第二个参数为3，表示一共可以处理4个请求（正在处理的一个+待处理的三个）
     */
    if (listen(listenfd, 5) != 0)
    {
        perror("listen");
        close(listenfd);
        return -1;
    }

    return listenfd;
}