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

CLogFile    logfile;            // 服务器日志文件
CTcpServer  TcpServer;          // 创建服务对象

pthread_spinlock_t vthidlock;   // 自旋锁，用于锁住多线程中vthid的使用
vector<pthread_t> vthid;        // 存放创建的线程id的容器

// 程序的帮助文档
void _help(void);
// xml参数解析为单独的参数
bool _xmltoarg(const char *xmlbuffer);
// 线程运行的主函数
void *thmain(void *arg);
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

#define MAXCONNS 10                 // 数据库连接池的大小
connection conns[MAXCONNS];         // 数据库连接池的数组
pthread_mutex_t mutex[MAXCONNS];    // 数据库连接池的锁
bool initconns();                   // 初始化数据库连接池
connection *getconns();             // 从数据库连接池中获取一个空闲的连接
bool freeconns(connection *conn);   // 释放/归还数据库连接
bool destroyconns();                // 释放数据库连接占用的资源（断开数据库连接，销毁锁）

int main(int argc, char *argv[])
{
    if (argc != 3) { _help(); return -1; }

    // 关闭标准输入输出和信号，只保留信号2(SIGINT)和信号15(SIGTERM)
    CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    if ( logfile.Open(argv[1], "a+") == false ) { printf("打开日志文件失败！\n"); return -1; }

    if (_xmltoarg(argv[2]) == false) return -1;

    if ( TcpServer.InitServer(starg.port) == false )
    { logfile.Write("服务器设置端口%d失败！\n", starg.port); return -1; }

    pthread_spin_init(&vthidlock, 0);

    // 初始化数据库连接池
    initconns();

    while (true)
    {
        if ( TcpServer.Accept() == false )
        { printf("Accept() failed!\n"); return -1; }
        // logfile.Write("客户端(%s)连接成功！\n", TcpServer.GetIP());

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
    int  connfd = (int)(long)arg;

    // 设置线程清理函数入栈
    pthread_cleanup_push(thcleanup, arg);
    // 设置为立即取消
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    // 分离线程
    pthread_detach(pthread_self());

    char strgetbuffer[1024];
    memset(strgetbuffer, 0, sizeof(strgetbuffer));
    // 读取客户端的报文，如果超时或者失败，线程退出
    /*
     * 需要超时判断，服务端不可能一直等，如果一直等，遇到了恶意的客户端，会消耗服务端资源
     * recv函数没有超时的判断
     * 不能使用框架中的函数，因为我们加了特有的报文头部
     * 因此只能自己动手写一个
     */
    if (ReadT(connfd, strgetbuffer, sizeof(strgetbuffer), 3) <= 0) pthread_exit(0);

    // 如果不是GET请求报文，不处理，线程退出
    if (strncmp(strgetbuffer, "GET", 3) != 0) pthread_exit(0);

    // logfile.Write("%s\n", strgetbuffer);

    // 从数据库连接池中获取一个数据库连接
    connection *conn = getconns();

    // 判断URL中用户名和密码，如果不正确，返回认证失败的响应报文，线程退出
    if (Login(conn, strgetbuffer, connfd) == false) { freeconns(conn); pthread_exit(0); }

    // 判断用户是否有调用接口的权限，如果没有，返回没有权限的响应报文，线程退出
    if (CheckPerm(conn, strgetbuffer, connfd) == false) { freeconns(conn); pthread_exit(0); }

    // 先把响应报文头部发送给客户端
    char strsendbuf[1024];
    memset(strsendbuf, 0, sizeof(strsendbuf));
    sprintf(strsendbuf,\
            "HTTP/1.1 200 OK\r\n"\
            "Server: webserver\r\n"\
            "Content-Type: text/html;charset=utf-8\r\n\r\n"\
    );
    Writen(connfd, strsendbuf, strlen(strsendbuf));

    // 在执行接口的sql语句，把数据返回给客户端
    if (ExecSQL(conn, strgetbuffer, connfd) == false) { freeconns(conn); pthread_exit(0); }

    freeconns(conn);

    pthread_cleanup_pop(1);
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
    // 关闭客户端的socketfd
    close((int)(long)arg);

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

    logfile.Write("线程%lu已退出\n", pthread_self());
}

// 进程的退出函数
void EXIT(int sig)
{
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    logfile.Write("接收到信号(%d)，进程即将推出\n", sig);

    TcpServer.CloseListen();

    pthread_spin_lock(&vthidlock);
    // printf("vthid.size() = %d\n", vthid.size());
    for (int ii = 0; ii < vthid.size(); ii ++)
    {
        pthread_cancel(vthid[ii]);
    }
    pthread_spin_unlock(&vthidlock);

    sleep(1);

    pthread_spin_destroy(&vthidlock);

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
*/
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