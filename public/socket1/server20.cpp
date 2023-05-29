/*
 * server20.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
 * 多线程服务端
 */
#include "../_public.h"

CLogFile    logfile;        // 服务器日志文件
CTcpServer  TcpServer;      // 创建服务对象

pthread_spinlock_t vthidlock;

vector<pthread_t> vthid;

void *thmain(void *arg);
void  thcleanup(void *arg);
void EXIT(int sig);         // 进程的退出函数

int main(int argc, char *argv[])
{
    if (argc != 3)
    { printf("Use:./server20 port logfile\nExample:./server20 5000 /tmp/server20.log\n"); return -1; }

    // 关闭标准输入输出和信号，只保留信号2(SIGINT)和信号15(SIGTERM)
    // CloseIOAndSignal(true);
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    if ( logfile.Open(argv[2], "a+") == false ) { printf("打开日志文件失败！\n"); return -1; }


    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { logfile.Write("服务器设置端口失败！\n"); return -1; }

    pthread_spin_init(&vthidlock, 0);

    while (true)
    {
        if ( TcpServer.Accept() == false )
        { printf("Accept() failed!\n"); return -1; }
        logfile.Write("客户端(%s)连接成功！\n", TcpServer.GetIP());

        pthread_t thid = 0;
        if (pthread_create(&thid, NULL, thmain, (void *)(long)TcpServer.m_connfd) != 0)
        {
            logfile.Write("创建线程失败\n");
            TcpServer.CloseListen();
            return 0;
        }
        pthread_spin_lock(&vthidlock);
        vthid.push_back(thid);
        pthread_spin_unlock(&vthidlock);
    }

    return 0;
}

void *thmain(void *arg)
{
    pthread_cleanup_push(thcleanup, arg);

    int connfd = (int)(long)arg;

    // 设置为立即取消
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    // 分离线程
    pthread_detach(pthread_self());

    int buflen;
    char buffer[1024];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (TcpRead(connfd, buffer, &buflen, 30) == false) break;
        logfile.Write("接受：%s\n", buffer);

        SPRINTF(buffer, sizeof(buffer), "ok");
        if (TcpWrite(connfd, buffer) == false) break;
        logfile.Write("发送：%s\n", buffer);
    }

    close(connfd);

    pthread_spin_lock(&vthidlock);
    // 在本线程执行完毕后，把本线程的id从容器中删除，避免EXIT函数中误删
    for (int ii = 0; ii < vthid.size(); ii ++)
    {
        // 这样写不合适，因为在不同的系统中，thid有的为无符号整数，有的为结构体
        // if (pthread_self() == vthid[ii])
        if (pthread_equal(pthread_self(), vthid[ii]))
        {
            vthid.erase(vthid.begin()+ii);
            break;
        }
    }
    pthread_spin_unlock(&vthidlock);

    pthread_cleanup_pop(1);
}

// 进程的退出函数
void EXIT(int sig)
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    logfile.Write("接收到信号(%d)，进程即将推出\n", sig);

    TcpServer.CloseListen();

printf("vthid.size() = %d\n", vthid.size());
    for (int ii = 0; ii < vthid.size(); ii ++)
    {
        pthread_cancel(vthid[ii]);
    }

    sleep(1);

    pthread_spin_destroy(&vthidlock);

    exit(0);
}

void thcleanup(void *arg)
{
    // 关闭客户端的socketfd
    close((int)(long)arg);

    logfile.Write("线程%lu已退出\n", pthread_self());
}
