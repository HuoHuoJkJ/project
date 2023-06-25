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
#include <sys/select.h>
#include <arpa/inet.h>

int initserver(int port);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Use:./tcpselect 5010\n");
        return -1;
    }

    /*
     * 初始化服务器
     */
    int listenfd;
    if ((listenfd = initserver(atoi(argv[1]))) < 0)
        printf("initserver(%s) failed!\n", argv[1]);
    printf("listenfd = %d\n", listenfd);

    fd_set readfds;         // 读事件socket的集合，包括监听socket和客户端连接上来的socet
    FD_ZERO(&readfds);      // 初始化读事件socket的集合
    FD_SET(listenfd, &readfds); // 把listenfd添加到读事件socket的集合中

    int maxfd = listenfd;   // 记录集合中socket的最大值

    while (true)
    {
        /*
         * 事件： 1）新客户端的连接请求accept；
         *       2）客户端连接已断开
         *       3）客户端有报文到达recv，可以读
         *       4）可以向客户端发送报文send，可以写
         * 可读事件  可写事件
         * select() 等待事件的发生（监视哪些socket发生了事件）
         */
        // select会修改XXXfds的bitmaps中的值，因此我们需要使用一个副本来作为select的参数
        struct timeval timeout;
        timeout.tv_sec = 5; timeout.tv_usec = 0;

        fd_set tmpfds = readfds;
        int infds = select(maxfd+1, &tmpfds, NULL, NULL, &timeout);

        // 处理select失败的情况
        if (infds < 0)
        {
            printf("select*() failed!\n");
            break;
        }

        // 超时，在本程序中，select函数最后一个参数为空，不存在超时的情况，但以下代码还是留着
        if (infds == 0)
        {
            printf("select() timeout!\n");
            continue;
        }

        // 如果infds>0，表示有事件发生的socket的数量
        for (int eventfd=0; eventfd<=maxfd; eventfd++)
        {
            // 如果没有事件，跳过本次循环
            if (FD_ISSET(eventfd, &tmpfds) <= 0) continue;

            if (eventfd == listenfd)
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

                // 把新客户端的sockfd加入可读sockfd的集合
                FD_SET(clientfd, &readfds);
                if (maxfd < clientfd)
                    maxfd = clientfd;
            }
            else
            {
                // 如果是客户端连接的sockfd事件，表示有报文发过来 或者 是连接已断开

                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                if (recv(eventfd, buffer, sizeof(buffer), 0) <= 0)
                {
                    printf("%ld client(eventfd=%d) disconnected.\n", time(0), eventfd);
                    close(eventfd);
                    FD_CLR(eventfd, &readfds);

                    if (eventfd == maxfd)
                    {
                        for (int ii=maxfd; ii>0; ii--)
                        {
                            if (FD_ISSET(ii, &readfds))
                            {
                                maxfd = ii;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    // printf("recv(eventfd=%d):%s\n", eventfd, buffer);

                    // fd_set tmpfds;
                    // FD_ZERO(&tmpfds);
                    // FD_SET(eventfd, &tmpfds);
                    // if (select(eventfd+1, NULL, &tmpfds, NULL, NULL) <= 0)
                    //     perror("select");
                    // else
                        send(eventfd, buffer, strlen(buffer), 0);
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