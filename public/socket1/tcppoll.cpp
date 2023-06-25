/*
 * 使用select模型，处理多个客户端的连接请求
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

// ulimit -a / -n 可以查看系统最大的文件描述符打开个数
#define MAXNFDS 1024

int initserver(int port);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Use:./tcppoll 5010\n");
        return -1;
    }

    /*
     * 初始化服务器
     */
    int listenfd;
    if ((listenfd = initserver(atoi(argv[1]))) < 0)
        printf("initserver(%s) failed!\n", argv[1]);
    printf("listenfd = %d\n", listenfd);

    struct pollfd fds[MAXNFDS]; // fds存放需要监视的sockfd
    // 初始化结构体中的fd为-1而不是0。在poll函数中，如果fd为-1，就会忽略它
    for (int ii = 0; ii < MAXNFDS; ii ++)
        fds[ii].fd = -1;

// 0  1  2  3  4  5  6  7  8
// 3  5  6 -1 -1 -1 -1 -1 -1
//-1 -1 -1  3 -1  5  6 -1 -1   采用这种方法
    fds[listenfd].fd = listenfd;
    fds[listenfd].events = POLLIN;

    int maxfd = listenfd;   // 记录集合中socket的最大值

    while (true)
    {
        int infds = poll(fds, maxfd+1, 5000);

        // 处理poll失败的情况
        if (infds < 0)
        {
            printf("poll*() failed!\n");
            break;
        }

        // 超时，在本程序中，poll函数最后一个参数为空，不存在超时的情况，但以下代码还是留着
        if (infds == 0)
        {
            printf("poll() timeout!\n");
            continue;
        }

        // 如果infds>0，表示有事件发生的poll的数量
        for (int eventfd=0; eventfd<=maxfd; eventfd++)
        {
            // 如果fd为-1，忽略它
            if (fds[eventfd].fd == -1) continue;

            // 如果没有事件，忽略它
            /*
             * revents 是一个位掩码，它表示已经发生的事件，例如可读（POLLIN）、可写（POLLOUT）等。& 是位运算中的按位与操作符，它将对应位置上的位进行逻辑与运算。
             * POLLIN 是一个宏定义，它表示文件描述符可读的事件。在Linux系统中，当一个文件描述符变为可读状态时，内核会设置 revents 中的相应位。因此，通过执行 revents & POLLIN，我们可以检查 eventfd 文件描述符是否可读，即检查 revents 的相应位是否被设置。
             * 如果 revents & POLLIN 的结果为非零值，表示 eventfd 文件描述符已经可读；如果结果为零，表示 eventfd 文件描述符不可读。
             */
            if ((fds[eventfd].revents&POLLIN) == 0) continue;

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

                // 修改fds数组中clientfd位置的元素
                fds[clientfd].fd = clientfd;
                fds[clientfd].events = POLLIN;
                fds[clientfd].revents = 0;

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
                    fds[eventfd].fd = -1;

                    if (eventfd == maxfd)
                    {
                        for (int ii=maxfd; ii>0; ii--)
                        {
                            if (fds[ii].fd != -1)
                            {
                                maxfd = ii;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    printf("recv(eventfd=%d):%s\n", eventfd, buffer);
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