/*
 * server.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
 */
#include "../_public.h"

bool SendHtmlFile(const int connfd, const char *filename);

int main(int argc, char *argv[])
{
    if (argc != 2)
    { printf("Use:./server30 port\nExample:./server30 5000\n"); return -1; }

    CTcpServer TcpServer;

    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { printf("服务器设置端口失败！\n"); return -1; }

    char buffer[1024];

    // if (fork() > 0) return 0;

    while (true)
    {
        if ( TcpServer.Accept() == false )
        { printf("Accept() failed!\n"); return -1; }
        printf("客户端(%s)连接成功！\n", TcpServer.GetIP());

        if (fork() > 0) continue;

        memset(buffer, 0, sizeof(buffer));
        recv(TcpServer.m_connfd, buffer, 1000, 0);
        printf("%s\n\n", buffer);

        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer,\
                "HTTP/1.1 200 OK\r\n"\
                "Server: webserver\r\n"\
                "Content-Type: text/html;charset=utf-8\r\n"\
                "\r\n");
                // "Content-Length: 105964\r\n"

        if (Writen(TcpServer.m_connfd, buffer, strlen(buffer)) == false) return -1;

        // SendHtmlFile(TcpServer.m_connfd, "/home/lighthouse/index.html");
        SendHtmlFile(TcpServer.m_connfd, "SURF_ZH_20211203151629_4944.json");

        return 0;
    }
    return 0;
}

bool SendHtmlFile(const int connfd, const char *filename)
{
    int bytes = 0;
    char buffer[5000];

    FILE *fp = NULL;

    if ((fp = FOPEN(filename, "rb")) == NULL) return false;

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));

        if ((bytes = fread(buffer, 1, sizeof(buffer), fp)) == 0) break;

        if (Writen(connfd, buffer, strlen(buffer)) == false)
        {
            fclose(fp);
            fp = NULL;
            return false;
        }
    }

    fclose(fp);

    return true;
}
