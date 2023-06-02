/*
 * server.cpp 作为服务端，接受客户端的数据，并向客户端发送数据
 */
#include "_public.h"
#include "_ooci.h"

bool SendData(const int connfd, const char *strget);
bool getvalue(const char *strget, const char *valuename, char *value, const int len);

/*
 * 接受客户端的请求报文
 * 解析URL中的参数，参数中指定了查询数据的条件
 * 从T_ZHOBTMIND1表中查询数据，以xml形式返回
 */

int main(int argc, char *argv[])
{
    if (argc != 2)
    { printf("Use:./server30 port\nExample:./server30 5000\n"); return -1; }

    CTcpServer TcpServer;

    if ( TcpServer.InitServer(atoi(argv[1])) == false )
    { printf("服务器设置端口失败！\n"); return -1; }

    if ( TcpServer.Accept() == false )
    { printf("Accept() failed!\n"); return -1; }
    printf("客户端(%s)连接成功！\n", TcpServer.GetIP());

    char strget[1024];

    memset(strget, 0, sizeof(strget));
    recv(TcpServer.m_connfd, strget, 1000, 0);
    printf("%s\n\n", strget);

    char strsend[1024];
    memset(strsend, 0, sizeof(strsend));
    sprintf(strsend,\
            "HTTP/1.1 200 OK\r\n"\
            "Server: webserver\r\n"\
            "Content-Type: text/html;charset=utf-8\r\n"\
            "\r\n");
            // "Content-Length: 105964\r\n"

    if (Writen(TcpServer.m_connfd, strsend, strlen(strsend)) == false) return -1;

    SendData(TcpServer.m_connfd, strget);

    return 0;
}

bool SendData(const int connfd, const char *strget)
{
    // 解析URL参数
    // 权限控制：用户名和密码
    // 接口名：访问数据的种类
    // 查询条件：设计接口的时候决定
    // http://127.0.0.1;8080/api?username=hhjkj&passwd=hhjkjpwd&intetname=getZHOBTMIND1&obti=123

    char username[31], passwd[31], intetname[31], obtid[11], begtime[21], endtime[21];
    memset(username, 0, sizeof(username));
    memset(passwd, 0, sizeof(passwd));
    memset(intetname, 0, sizeof(intetname));
    memset(obtid, 0, sizeof(obtid));
    memset(begtime, 0, sizeof(begtime));
    memset(endtime, 0, sizeof(endtime));

    getvalue(strget, "username", username, 30);
    getvalue(strget, "passwd", passwd, 30);
    getvalue(strget, "intetname", intetname, 30);
    getvalue(strget, "obtid", obtid, 10);
    getvalue(strget, "begtime", begtime, 20);
    getvalue(strget, "endtime", endtime, 20);

    printf("username = %s, passwd = %s, intetname = %s, obtid = %s, begtime = %s, endtime = %s\n", username, passwd, intetname, obtid, begtime, endtime);

    connection conn;
    conn.connecttodb("qxidc/dyting9525@snorcl11g_43", "Simplified Chinese_China.AL32UTF8");
    char strxml[10240];

    // 查询数据库表中符合标准的数据

    sqlstatement stmt(&conn);
    stmt.prepare("select '<obtid>'||obtid||'</obtid>'||'<ddatetime>'||to_char(ddatetime,'yyyy-mm-dd hh24:mi:ss')||'</ddatetime>'||'<t>'||t||'</t>'||'<p>'||p||'</p>'||'<u>'||u||'</u>'||'<keyid>'||keyid||'</keyid>'||'<endl/>' from T_ZHOBTMIND1 where obtid=:1 and ddatetime>to_date(:2,'yyyymmddhh24miss') and ddatetime<to_date(:3,'yyyymmddhh24miss')");
    // stmt.prepare("select '<obtid>'||obtid||'</obtid>'||'<ddatetime>'||to_char(ddatetime,'yyyy-mm-dd hh24:mi:ss')||'</ddatetime>'||'<t>'||t||'</t>'||'<p>'||p||'</p>'||'<u>'||u||'</u>'||'<keyid>'||keyid||'</keyid>'||'<endl/>' from T_ZHOBTMIND1 where obtid=51068 and ddatetime>to_date(20230601181000,'yyyymmddhh24miss')  and ddatetime<to_date(20230601184000,'yyyymmddhh24miss')");
    stmt.bindin(1, obtid, 10);
    stmt.bindin(2, begtime, 14);
    stmt.bindin(3, endtime, 14);
    stmt.bindout(1, strxml, 10000);
    stmt.execute();

    // 将数据发送给客户端
    Writen(connfd, "<data>\n", strlen("<data>\n"));

    while (true)
    {
        memset(strxml, 0, sizeof(strxml));
        if (stmt.next() != 0) break;

        strcat(strxml, "\n");
        Writen(connfd, strxml, strlen(strxml));
    }

    Writen(connfd, "</data>\n", strlen("</data>\n"));

    return true;
}

bool getvalue(const char *strget, const char *valuename, char *value, const int len)
{
    char *start, *end;
    if ((start = strstr((char *)strget, (char *)valuename)) == NULL) return false;

    if ((end = strstr(start, "&")) == NULL)
        end = strstr(start, " ");
    if (end == NULL) return false;

    int ilen = end - (start + strlen(valuename) + 1);
    // int ilen = (strlen(start) - (strlen(end) + strlen(valuename) + 1));
    if (ilen > len) ilen = len;

    strncpy(value, start + strlen(valuename) + 1, ilen);

    return true;
}