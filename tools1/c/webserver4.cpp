/*
 * server20.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
 * 多线程服务端
 */
#include "_public.h"
#include "_ooci.h"

struct st_args
{
    char connstr[101];  // 数据库连接参数
    char charset[51];   // 数据库字符集
    int  port;          // webserver监听端口
} starg;

class connpool
{
private:
    struct st_conn
    {
        connection conn;        // 数据库连接参数
        pthread_mutex_t mutex;  // 用于数据库连接的互斥锁
        time_t atime;           // 数据库连接上次使用的时间，如果未连接数据库则取值为0
    } *m_conns;

    int  m_maxconns;            // 数据库连接池的最大值
    int  m_timeout;             // 数据库连接超时时间，单位：秒
    char m_connstr[101];        // 数据库的连接参数
    char m_charset[101];        // 数据库的字符集
public:
    connpool();
   ~connpool();

    // 初始化数据库连接池，初始化锁，如果数据库连接参数有问题，返回false
    bool init(const char *connstr, const char *charset, int maxconns, int timeout);
    // 断开数据库连接，销毁锁，释放数据库连接池的内存空间
    void destroy();

    // 从数据库连接池中获取一个空闲的连接，成功返回数据库连接的地址
    // 如果连接池已用完或者数据库连接失败，返回空
    connection *get();
    // 归还数据库连接
    bool free(connection *conn);

    // 检查数据库连接池，断开空闲的连接，在服务程序中，用一个专用的子线程调用此函数
    void checkpool();
};

CLogFile    logfile;            // 服务器日志文件
CTcpServer  TcpServer;          // 创建服务对象
connpool    oraconnpool;        // 数据库连接池

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // 声明并初始化互斥锁
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;   // 声明并初始化条件变量
vector<int>     socket_queue;                       // 线程处理的客户端的socket的队列

pthread_spinlock_t spin;        // 自旋锁，用于锁住多线程中vthid的使用
vector<pthread_t> vthid;        // 存放创建的线程id的容器

// 程序的帮助文档
void _help(void);
// xml参数解析为单独的参数
bool _xmltoarg(const char *xmlbuffer);
// 线程运行的主函数
void *thmain(void *arg);
// 检查数据库连接池是否超时的线程的主函数
void *checkpool(void *arg);
// 能够判断超时的接受客户端报文的函数
int  ReadT(const int sockfd, char *buffer, const int size, const int itimeout);
// 判断URL中用户名和密码，如果不正确，返回认证失败的响应报文，线程退出
bool Login(connection *conn, const char *buffer, const int sockfd);
// 判断用户是否有调用接口的权限，如果没有，返回没有权限的响应报文，线程退出
bool CheckPerm(connection *conn, const char *buffer, const int sockfd);
// 在执行接口的sql语句，把数据返回给客户端
bool ExecSQL(connection *conn, const char *buffer, const int sockfd);
// 解析URL中的字符串的变量
bool GetValue(const char *buffer, const char *valuename, char *value, int len);
// 线程结束时的清理函数
void  thcleanup(void *arg);
// 进程的退出函数
void EXIT(int sig);

int main(int argc, char *argv[])
{
    if (argc != 3) { _help(); return -1; }

    // 关闭标准输入输出和信号，只保留信号2(SIGINT)和信号15(SIGTERM)
    // CloseIOAndSignal(true);
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    if ( logfile.Open(argv[1], "a+") == false ) { printf("打开日志文件失败！\n"); return -1; }

    if (_xmltoarg(argv[2]) == false) return -1;

    if ( TcpServer.InitServer(starg.port) == false )
    { logfile.Write("服务器设置端口%d失败！\n", starg.port); return -1; }

    pthread_spin_init(&spin, 0);

    // 初始化数据库连接池
    if (oraconnpool.init(starg.connstr, starg.charset, 10, 50) == false)
    {
        logfile.Write("oraconnpool.init() failed.\n");
        return -1;
    }
    else
    {
        pthread_t thid = 0;
        if (pthread_create(&thid, NULL, checkpool, NULL) != 0)
        {
            logfile.Write("创建线程失败\n");
            return -1;
        }
    }

    // 创建10个线程，线程数略多于cpu核数
    for (int ii = 0; ii < 10; ii ++)
    {
        pthread_t thid = 0;
        if (pthread_create(&thid, NULL, thmain, (void *)(long)ii) != 0)
        {
            logfile.Write("创建线程失败\n");
            return -1;
        }
        vthid.push_back(thid);
    }

    while (true)
    {
        if ( TcpServer.Accept() == false )
        { printf("Accept() failed!\n"); return -1; }
        logfile.Write("客户端(%s)连接成功！\n", TcpServer.GetIP());

        // 把客户端的socket放入队列，并发送信号（需要互斥锁、信号量、socket队列容器）
        pthread_mutex_lock(&mutex);                 // 加锁
        socket_queue.push_back(TcpServer.m_connfd);  // 入队
        pthread_mutex_unlock(&mutex);               // 解锁
        pthread_cond_signal(&cond);                 // 触发条件，激活下一线程
    }

    return 0;
}

// 程序的帮助文档
void _help(void)
{
    printf("Using:webserver logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools1/bin/procctl 10 /project/tools1/bin/webserver /log/idc/webserver.log \"<connstr>scott/dyting9525@snorcl11g_43</connstr><charset>Simplified Chinese_China.AL32UTF8</charset><port>8080</port>\"\n\n");

    printf("本程序是数据总线的服务端程序，为数据中心提供http协议的数据访问接口。\n");
    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：username/password@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("port        web服务监听的端口。\n\n");
}

// xml参数解析为单独的参数
bool _xmltoarg(const char *xmlbuffer)
{
    memset(&starg,0,sizeof(struct st_args));

    GetXMLBuffer(xmlbuffer,"connstr",starg.connstr,100);
    if (strlen(starg.connstr)==0) { logfile.Write("connstr is null.\n"); return false; }

    GetXMLBuffer(xmlbuffer,"charset",starg.charset,50);
    if (strlen(starg.charset)==0) { logfile.Write("charset is null.\n"); return false; }

    GetXMLBuffer(xmlbuffer,"port",&starg.port);
    if (starg.port==0) { logfile.Write("port is null.\n"); return false; }

    return true;
}

void *thmain(void *arg)
{
    // 给本线程起一个能看懂的编号1-10，而不是thid
    int pthnum = (int)(long)arg;
    int confd;

    // 设置线程清理函数入栈
    pthread_cleanup_push(thcleanup, arg);
    // 设置为立即取消
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    // 分离线程
    pthread_detach(pthread_self());

    char strgetbuffer[1024];
    char strsendbuf[1024];

    while (true)
    {
        // 给缓存队列加锁
        pthread_mutex_lock(&mutex);

        // 如果socket队列为空，等待，用while条件防止条件变量被虚假唤醒
        while (socket_queue.size() == 0)
            pthread_cond_wait(&cond,&mutex);

        // 从队列中取出sockfd连接，在该队列中删除该连接
        confd = socket_queue[0];
        socket_queue.erase(socket_queue.begin());

        // 给队列解锁
        pthread_mutex_unlock(&mutex);

        /*
         * 下面是业务处理的代码
         */
        logfile.Write("phid=%ld(num=%d), confd=%d\n", pthread_self(), pthnum, confd);

        // 读取客户端的报文，如果超时或者失败，线程退出
        /*
         * 需要超时判断，服务端不可能一直等，如果一直等，遇到了恶意的客户端，会消耗服务端资源
         * recv函数没有超时的判断 不能使用框架中的函数，因为我们加了特有的报文头部 因此只能自己动手写一个
         */
        if (ReadT(confd, strgetbuffer, sizeof(strgetbuffer), 3) <= 0) { close(confd); continue; }

        // 如果不是GET请求报文，不处理，线程退出
        if (strncmp(strgetbuffer, "GET", 3) != 0) { close(confd); continue; }

        logfile.Write("%s\n", strgetbuffer);

        // 从数据库连接池中获取一个数据库连接
        connection *conn = oraconnpool.get();

        if (conn == NULL)
        {
            memset(strsendbuf, 0, sizeof(strsendbuf));
            sprintf(strsendbuf,\
                    "HTTP/1.1 200 OK\r\n"\
                    "Server: webserver\r\n"\
                    "Content-Type: text/html;charset=utf-8\r\n\r\n"\
                    "<retcode>-1</retcode><message>internal error.</message>"
                   );
            Writen(confd, strsendbuf, strlen(strsendbuf));
            close(confd); continue;
        }

        // 判断URL中用户名和密码，如果不正确，返回认证失败的响应报文，线程退出
        if (Login(conn, strgetbuffer, confd) == false) { oraconnpool.free(conn); close(confd); continue; }

        // 判断用户是否有调用接口的权限，如果没有，返回没有权限的响应报文，线程退出
        if (CheckPerm(conn, strgetbuffer, confd) == false) { oraconnpool.free(conn); close(confd); continue; }

        // 先把响应报文头部发送给客户端
        memset(strsendbuf, 0, sizeof(strsendbuf));
        sprintf(strsendbuf,\
                "HTTP/1.1 200 OK\r\n"\
                "Server: webserver\r\n"\
                "Content-Type: text/html;charset=utf-8\r\n\r\n"\
        );
        Writen(confd, strsendbuf, strlen(strsendbuf));

        // 在执行接口的sql语句，把数据返回给客户端
        if (ExecSQL(conn, strgetbuffer, confd) == false) { oraconnpool.free(conn); close(confd); continue; }

        oraconnpool.free(conn);

    }

    pthread_cleanup_pop(1);
}

// 检查数据库连接池是否超时的线程的主函数
void *checkpool(void *arg)
{
    while (true)
    {
        oraconnpool.checkpool(); sleep(30);
    }
}

// 能够判断超时的接受客户端报文的函数
int   ReadT(const int sockfd, char *buffer, const int size, const int itimeout)
{
    if (itimeout > 0)
    {
        struct pollfd fds;
        fds.fd = sockfd;
        fds.events = POLLIN;
        int iret;
        if ((iret = poll(&fds, 1, itimeout*1000)) <= 0) return iret;
    }

    return recv(sockfd, buffer, size, 0);
}

// 判断URL中用户名和密码，如果不正确，返回认证失败的响应报文，线程退出
bool Login(connection *conn, const char *buffer, const int sockfd)
{
    char username[31], passwd[31];
    int  icount = 0;

    GetValue(buffer, "username", username, 30);
    GetValue(buffer, "passwd", passwd, 30);

    sqlstatement stmt(conn);
    stmt.prepare("select count(*) from T_USERINFO where username=:1 and passwd=:2 and rsts=1");
    stmt.bindin(1, username, 30);
    stmt.bindin(2, passwd, 30);
    stmt.bindout(1, &icount);
    stmt.execute();
    stmt.next();

    if (icount == 0)
    {
        char strbuffer[256];
        memset(strbuffer, 0, sizeof(strbuffer));

        sprintf(strbuffer,\
                "HTTP/1.1 200 OK\r\n"\
                "Server: webserver\r\n"\
                "Content-Type: text/html;charset=utf-8\r\n\r\n"\
                "<retcode>-1</retcode><message>username or passwd is invailed</message>");
        Writen(sockfd, strbuffer, strlen(strbuffer));

        return false;
    }

    return true;
}

// 判断用户是否有调用接口的权限，如果没有，返回没有权限的响应报文，线程退出
bool CheckPerm(connection *conn, const char *buffer, const int sockfd)
{
    char username[31], intername[31];
    int  icount = 0;

    GetValue(buffer, "username", username, 30);
    GetValue(buffer, "intername", intername, 30);

    sqlstatement stmt(conn);
    stmt.prepare("select count(*) from T_USERANDINTER where username=:1 and intername=:2 and intername in (select intername from T_INTERCFG where rsts=1)");
    stmt.bindin(1, username, 30);
    stmt.bindin(2, intername, 30);
    stmt.bindout(1, &icount);
    stmt.execute();
    stmt.next();

    if (icount == 0)
    {
        char strbuffer[256];
        memset(strbuffer, 0, sizeof(strbuffer));

        sprintf(strbuffer,\
                "HTTP/1.1 200 OK\r\n"\
                "Server: webserver\r\n"\
                "Content-Type: text/html;charset=utf-8\r\n\r\n"\
                "<retcode>-1</retcode><message>permission denied</message>");
        Writen(sockfd, strbuffer, strlen(strbuffer));

        return false;
    }

    return true;
}

// 在执行接口的sql语句，把数据返回给客户端
bool ExecSQL(connection *conn, const char *buffer, const int sockfd)
{
    char intername[31];
    memset(intername, 0, sizeof(intername));
    // 从请求报文中解析接口名
    GetValue(buffer, "intername", intername, 30);

    char selectsql[1001], colstr[301], bindin[301];
    memset(selectsql, 0, sizeof(selectsql));
    memset(colstr, 0, sizeof(colstr));
    memset(bindin, 0, sizeof(bindin));
    // 从接口参数配置表T_INTERCFG中加载接口参数
    sqlstatement stmt(conn);
    stmt.prepare("select selectsql,colstr,bindin from T_INTERCFG where intername=:1");
    stmt.bindin(1, intername, 30);
    stmt.bindout(1, selectsql, 1000);
    stmt.bindout(2, colstr, 300);
    stmt.bindout(3, bindin, 300);
    stmt.execute();
    stmt.next();

    // 准备查询数据的SQL语句
    stmt.prepare(selectsql);
    // stmt.prepare(
        // "select obtid,to_char(ddatetime,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis from T_ZHOBTMIND2 where obtid=59287 and ddatetime>=to_date(20230530104800,'yyyymmddhh24miss') and ddatetime<=to_date(20230602211148,'yyyymmddhh24miss')"
    // );
    // logfile.Write("\n%s\n", selectsql);
    // logfile.Write("\n%s\n", colstr);
    // logfile.Write("\n%s\n", bindin);

    // http://43.143.136.101:8080/?username=ty&passwd=typwd&intername=getzhobtmind&obtid=59287&begtime=202306211048&endtime=20230601211148
    // SQL语句： select obtid,to_char(ddatetime,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis from T_ZHOBTMIND1 where obtid=:1 and ddatetime>=to_date(:2,'yyyymmddhh24miss') and ddatetime<=to_date(:3,'yyyymmddhh24miss')
    // colstr字段：obtid,ddatetime,t,p,u,wd,wf,r,vis
    // bindin字段：obtid,begintime,endtime

    // 绑定查询数据的SQL语句的输入变量
    // 根据接口配置中的参数列表（bindin字段），从URL中解析出参数的值，绑定到查询数据的SQL语句中
    //////////////////////////////////////////////////////////////////////////////////
    // 拆分输入参数bindin
    CCmdStr CmdStr;
    CmdStr.SplitToCmd(bindin, ",");

    // 声明用于存放输入参数的数据，输入参数的值不会太长，100足够
    char invalue[CmdStr.CmdCount()][101];
    memset(invalue, 0, sizeof(invalue));

    // 从http的GET请求报文中解析出输入参数，绑定到sql中
    for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
    {
        // logfile.Write("%s\n", CmdStr.m_vCmdStr[ii].c_str());
        GetValue(buffer, CmdStr.m_vCmdStr[ii].c_str(), invalue[ii], 100);
        stmt.bindin(ii+1, invalue[ii], 100);
    }
    //////////////////////////////////////////////////////////////////////////////////

    // 绑定查询数据的SQL语句的输出变量
    // 根据接口配置中的列名（colstr字段），bindout结果集
    //////////////////////////////////////////////////////////////////////////////////
    CmdStr.SplitToCmd(colstr, ",");

    // 声明用于存放输出参数的数据，输出参数的值不会太长，2000足够
    char colvalue[CmdStr.CmdCount()][2001];
    memset(colvalue, 0, sizeof(colvalue));

    // 从http的GET请求报文中解析出输出参数，绑定到sql中
    for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
    {
        // logfile.Write("%s\n", CmdStr.m_vCmdStr[ii].c_str());
        stmt.bindout(ii+1, colvalue[ii], 2000);
    }
    //////////////////////////////////////////////////////////////////////////////////

    char strsendbuffer[4001];
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    // 执行SQL语句
    if (stmt.execute() != 0)
    {
        sprintf(strsendbuffer, "<retcode>%d</retcode><message>%s</message>", stmt.m_cda.rc, stmt.m_cda.message);
        Writen(sockfd, strsendbuffer, strlen(strsendbuffer));
        logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return false;
    }
    Writen(sockfd, "<retcode>0</retcode><message>ok</message>\n", strlen("<retcode>0</retcode><message>ok</message>\n"));

    // 向客户端发送xml内容的头部标签<data>
    Writen(sockfd, "<data>\n", strlen("<data>\n"));

    char strtemp[2001];
    // 获取结果集，每获取一条记录，拼接xml报文。发送给客户端
    while (true)
    {
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        memset(colvalue, 0, sizeof(colvalue));

        if (stmt.next() != 0) break;

        for (int ii = 0; ii < CmdStr.CmdCount(); ii ++)
        {
            memset(strtemp, 0, sizeof(strtemp));
            snprintf(strtemp, 2000, "<%s>%s</%s>", CmdStr.m_vCmdStr[ii].c_str(), colvalue[ii], CmdStr.m_vCmdStr[ii].c_str());
            strcat(strsendbuffer, strtemp);
        }

        strcat(strsendbuffer, "<endl/>\n");
        Writen(sockfd, strsendbuffer, strlen(strsendbuffer));
    }

    // 向客户端发送xml内容的尾部标签</data>
    Writen(sockfd, "</data>\n", strlen("</data>\n"));

    logfile.Write("intername=%s, count=%d\n", intername, stmt.m_cda.rpc);
    // 写接口调用日志表T_USERLOG

    return true;
}

// 解析URL中的字符串的变量
bool GetValue(const char *buffer, const char *valuename, char *value, int len)
{
    char *start = NULL, *end = NULL;

    start = strstr((char *)buffer, (char *)valuename);
    if (start == NULL) return false;

    end = strstr(start, "&");
    if (end == NULL) end = strstr(start, " ");
    if (end == NULL) return false;

    int ilen = 0;
    ilen = strlen(start) - (strlen(end) + strlen(valuename) + 1);
    if (ilen > len) ilen = len;

    strncpy(value, start+strlen(valuename)+1, ilen);
    // logfile.Write("%s\n", value);

    return true;
}

void thcleanup(void *arg)
{
    pthread_spin_lock(&spin);
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
    pthread_spin_unlock(&spin);

    logfile.Write("线程%lu(%d)已退出\n", pthread_self(), (int)(long)arg);
}

// 进程的退出函数
void EXIT(int sig)
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    logfile.Write("接收到信号(%d)，进程即将推出\n", sig);

    TcpServer.CloseListen();

    pthread_spin_lock(&spin);
    // printf("vthid.size() = %d\n", vthid.size());
    for (int ii = 0; ii < vthid.size(); ii ++)
    {
        pthread_cancel(vthid[ii]);
    }
    pthread_spin_unlock(&spin);

    sleep(1);

    pthread_spin_destroy(&spin);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    exit(0);
}
/*
#define MAXCONNS 10                 // 数据库连接池的大小
connection conns[MAXCONNS];         // 数据库连接池的数组
pthread_mutex_t mutex[MAXCONNS];    // 数据库连接池的锁
bool initconns();                   // 初始化数据库连接池
connection *getconns();             // 从数据库连接池中获取一个空闲的连接
bool freeconns(connection *conn);   // 释放/归还数据库连接
bool destroyconns();                // 释放数据库连接占用的资源（断开数据库连接，销毁锁）
// 初始化数据库连接池
bool initconns()
{
    for (int ii = 0; ii < MAXCONNS; ii ++)
    {
        // 初始化数据库
        if (conns[ii].connecttodb(starg.connstr, starg.charset) != 0)
        {
            logfile.Write("conns.connecttodb failed!\n%s\n", conns[ii].m_cda.message);
            return false;
        }

        // 初始化锁
        pthread_mutex_init(&mutex[ii], NULL);
    }

    return true;
}

// 从数据库连接池中获取一个空闲的连接
connection *getconns()
{
    // 如果获取数据库连接失败，则一直重试，直到获取成功为止
    while (true)
    {
        for (int ii = 0; ii < MAXCONNS; ii ++)
        {
            if (pthread_mutex_trylock(&mutex[ii]) == 0)
            {
                logfile.Write("get conns is %d.\n", ii);
                return &conns[ii];
            }
        }
        usleep(100000); // 休眠十分之一秒再继续
    }
}

// 释放/归还数据库连接
bool freeconns(connection *conn)
{
    for (int ii = 0; ii < MAXCONNS; ii ++)
    {
        if (&conns[ii] == conn)
        {
            pthread_mutex_unlock(&mutex[ii]);
            conn = 0;
            return true;
        }
    }

    return false;
}

// 释放数据库连接占用的资源（断开数据库连接，销毁锁）
bool destroyconns()
{
    for (int ii = 0; ii < MAXCONNS; ii ++)
    {
        conns[ii].disconnect();
        pthread_mutex_destroy(&mutex[ii]);
    }

    return true;
}
*/

/*
class connpool
{
private:
    struct st_conn
    {
        connection conn;        // 数据库连接参数
        pthread_mutex_t mutex;  // 用于数据库连接的互斥锁
        time_t atime;           // 数据库连接上次使用的时间，如果未连接数据库则取值为0
    } *m_conn;

    int  m_maxconns;            // 数据库连接池的最大值
    int  m_timeout;             // 数据库连接超时时间，单位：秒
    char m_connstr[101];        // 数据库的连接参数
    char m_charset[101];        // 数据库的字符集
public:
    connpool();
   ~connpool();

    // 初始化数据库连接池，初始化锁，如果数据库连接参数有问题，返回false
    bool init(const char *connstr, const char *charset, int maxconns, int timeout);
    // 断开数据库连接，销毁锁，释放数据库连接池的内存空间
    void destroy();

    // 从数据库连接池中获取一个空闲的连接，成功返回数据库连接的地址
    // 如果连接池已用完或者数据库连接失败，返回空
    connection *get();
    // 归还数据库连接
    void free(connection *conn);

    // 检查数据库连接池，断开空闲的连接，在服务程序中，用一个专用的子线程调用此函数
    void checkpool();
}
*/
connpool::connpool()
{
    m_maxconns = 0;
    m_timeout = 0;
    memset(m_connstr, 0, sizeof(m_connstr));
    memset(m_charset, 0, sizeof(m_charset));
    m_conns = 0;
}

connpool::~connpool()
{
    destroy();
}

// 初始化数据库连接池，初始化锁，如果数据库连接参数有问题，返回false
bool connpool::init(const char *connstr, const char *charset, int maxconns, int timeout)
{
    connection conn;
    // 尝试连接数据库，验证数据库连接参数是否正确
    if (conn.connecttodb(connstr, charset) != 0)
    {
        printf("连接数据库失败。\n%s\n", conn.m_cda.message);
        return false;
    }
    conn.disconnect();

    strncpy(m_connstr, connstr, sizeof(m_connstr)-1);
    strncpy(m_charset, charset, sizeof(m_charset)-1);
    m_maxconns = maxconns;
    m_timeout = timeout;

    // 分配数据库连接池内存空间
    m_conns = new struct st_conn[m_maxconns];

    // 初始化结构体，conn不需要初始化
    for (int ii = 0; ii < m_maxconns; ii ++)
    {
        pthread_mutex_init(&m_conns[ii].mutex, NULL);
        m_conns[ii].atime = 0;
    }

    return true;
}

// 断开数据库连接，销毁锁，释放数据库连接池的内存空间
void connpool::destroy()
{
    for (int ii = 0; ii < m_maxconns; ii ++)
    {
        m_conns[ii].conn.disconnect();              // 断开数据库连接
        pthread_mutex_destroy(&m_conns[ii].mutex);  // 销毁锁
    }
    delete []m_conns;       // 释放数据库连接池的内存空间
    m_conns = 0;

    memset(m_connstr, 0, sizeof(m_connstr));
    memset(m_charset, 0, sizeof(m_charset));
    m_maxconns = 0;
    m_timeout = 0;
}

// 1）从数据库连接池中寻找一个空闲的、已连接好的connection，如果找到了，返回它的地址
// 2）如果没有找到，在连接池中找一个未连接数据库的connection，连接数据库，如果成功，返回connection的地址
// 3）如果2）步找到了未连接数据库的connection，但是，连接数据库失败，返回空
// 4）如果2）步没有找到未连接数据库的connection，表示数据库连接池已用完，也返回空
connection *connpool::get()
{
    int pos = -1;       // 用于记录第一个未连接数据库的数组位置

    // 通过判断能否加锁，来确定数据库是否空闲
    for (int ii = 0; ii < m_maxconns; ii ++)
    {
        // 如果这条if语句成立，则能够找到一个空闲的、已经连接好的connection
        if (pthread_mutex_trylock(&m_conns[ii].mutex) == 0)
        {
            // 如果数据库连接是已连接状态
            if (m_conns[ii].atime > 0)
            {
                printf("取到连接%d\n", ii);
                m_conns[ii].atime = time(0);    // 把数据库连接的使用时间设置为当前时间
                return &m_conns[ii].conn;       // 返回数据库连接的地址
            }

            /*
             * 这里，如果找到了一个未连接数据库的位置，不能释放锁，因为如果释放了锁，可能会有多个线程同时找到了这个位置，会引起多次数据库连接
             */
            if (pos == -1)                      // 记录第一个未连接数据库的数组位置
                pos = ii;
            else
                pthread_mutex_unlock(&m_conns[ii].mutex);   // 如果数据库不是已连接状态，这里需要释放锁
        }
    }

    // 如果循环结束，pos的值仍然为-1，说明此时数据库不存在未连接数据库的connection，也就说明，数据库连接池已用完
    if (pos == -1)
    {
        printf("数据库连接池已用完\n");
        return NULL;
    }

    // 此时说明数据库连接池中存在未连接的connection
    // pthread_mutex_lock(&m_conns[pos].mutex);
    // 尝试连接数据库
    printf("新连接%d\n", pos);

    if (m_conns[pos].conn.connecttodb(m_connstr, m_charset) != 0)
    {
        // 连接失败，释放锁，返回空
        printf("连接数据库失败\n");
        // pthread_mutex_unlock(&m_conns[pos].mutex);
        return NULL;
    }
    m_conns[pos].atime = time(0);

    return &m_conns[pos].conn;
}

// 归还数据库连接
bool connpool::free(connection *conn)
{
    /*
     * 用一个循环，遍历数组，找到对应的conn，释放锁
     */
    for (int ii = 0; ii < m_maxconns; ii ++)
    {
        if (conn == &m_conns[ii].conn)
        {
            printf("归还%d\n", ii);
            m_conns[ii].atime = time(0);
            pthread_mutex_unlock(&m_conns[ii].mutex);
            return true;
        }
    }
    return false;
}

// 检查数据库连接池，断开空闲的连接，在服务程序中，用一个专用的子线程调用此函数
void connpool::checkpool()
{
    for (int ii = 0; ii < m_maxconns; ii ++)
    {
        if (pthread_mutex_trylock(&m_conns[ii].mutex) == 0)
        {
            if (m_conns[ii].atime > 0)
            {
                if (m_timeout < (time(0) - m_conns[ii].atime))
                {
                    printf("连接%d已超时\n", ii);
                    m_conns[ii].conn.disconnect();
                    m_conns[ii].atime = 0;
                }
                else
                {
                    // 如果没有超时，执行一次sql，验证连接是否有效，如果无效，断开它
                    // 如果网络断开了，或者数据库重启了，那么就需要重新连接数据库，在这里，只需要断开连接就行啦
                    // 重连的操作交给get()函数
                    if (m_conns[ii].conn.execute("select * from dual") != 0)
                    {
                        printf("连接%d已故障\n", ii);
                        m_conns[ii].conn.disconnect();
                        m_conns[ii].atime = 0;
                    }
                }
            }
            pthread_mutex_unlock(&m_conns[ii].mutex);
        }
        // 如果尝试加锁失败，说明此时有线程正在使用数据库连接池中的connection，不必检查是否超时
    }
}