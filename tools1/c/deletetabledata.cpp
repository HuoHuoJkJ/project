/*
 * 本程序是数据中心的公共功能模块，用于定时清理数据
 */
#include "_public.h"
#include "_mysql.h"

struct st_arg
{
    char connectstr[101];   // 数据库连接参数
    char tname[51];         // 表名
    char keycol[31];        // 待清理的表的唯一键字段名
    char where[1001];       // 需要清理数据的条件
    char starttime[41];         // 程序的开始时间(小时)。一般选择在数据库负载压力比较小的时间段进行，必须夜晚等。如果该参数为空，则立即执行数据清理
    int  timeout;           // 程序心跳的超时时间
    char pname[51];         // 程序心跳的名称
} starg;

CLogFile    logfile;
CPActive    PActive;
connection  conn1;          // 用于执行查询sql语句的数据库连接
connection  conn2;          // 用于执行删除sql语句的数据库连接

// 程序的帮助文档
void _help();
// 解析xmlbuffer参数
bool _xmltoarg(char *xmlbuffer);
// 判断当前时间是否符合starttime的标准
bool _isstarttime();
// 数据清理主函数
bool _deletetabledata();
// 程序的退出函数
void _exit(int sig);

int main(int argc, char *argv[])
{
    if (argc != 3) { _help(); return 0; }

    // CloseIOAndSignal(true); signal(SIGINT, _exit); signal(SIGTERM, _exit);

    if (logfile.Open(argv[1], "a+") == false) { printf("打开日志文件%s失败\n", argv[1]); return -1; }

    if (_xmltoarg(argv[2]) == false) return -1;

    // 判断当前时间是否符合starttime的标准
    if (_isstarttime() == false) return 0;

    // PActive.AddPInfo(starg.timeout, starg.pname);
    PActive.AddPInfo(5000, starg.pname);

    if (conn1.connecttodb(starg.connectstr, NULL) != 0)
    { logfile.Write("执行select语句的数据库连接失败(%s)\n%s\n", starg.connectstr, conn1.m_cda.message); _exit(-1); }

    if (conn2.connecttodb(starg.connectstr, NULL, 1) != 0)  // 开启自动提交
    { logfile.Write("执行select语句的数据库连接失败(%s)\n%s\n", starg.connectstr, conn2.m_cda.message); _exit(-1); }

    if (_deletetabledata() == false) _exit(-1);

    return 0;
}

// 程序的帮助文档
void _help()
{
    printf("\n");
    printf("Use:\n");
    printf("deletetabledata logfile xmlbuffer");
    printf("Example:\n");
    printf("/project/tools1/bin/procctl 3600 /project/tools1/bin/deletetabledata /log/idc/deletetabledata_T_ZHOBTMIND1.log \"<connectstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connectstr><tname>T_ZHOBTMIND1</tname><keycol>keyid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><timeout>120</timeout><pname>deletetabledata_ZHOBTMIND1</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，用于定时清理数据\n");

    printf("logfile     本程序用于记录日志的文件\n");
    printf("xmlbuffer   本程序的运行参数\n");

    printf("connectstr  数据库连接参数\n");
    printf("tname       表名\n");
    printf("keycol      待清理的表的唯一键字段名\n");
    printf("where       需要清理数据的条件\n");
    printf("starttime   程序的开始时间(小时)。一般选择在数据库负载压力比较小的时间段进行，必须夜晚等。如果该参数为空，则立即执行数据清理\n");
    printf("timeout     程序心跳的超时时间\n");
    printf("pname       程序心跳的名称\n");
    printf("\n");
}

// 解析xmlbuffer参数
bool _xmltoarg(char *xmlbuffer)
{
    memset(&starg, 0, sizeof(starg));

    // 数据库连接参数
    GetXMLBuffer(xmlbuffer, "connectstr", starg.connectstr, 100);
    if (strlen(starg.connectstr) == 0)
    { logfile.Write("connectstr 参数为空\n"); return false; }
    // 表名
    GetXMLBuffer(xmlbuffer, "tname", starg.tname, 51);
    if (strlen(starg.tname) == 0)
    { logfile.Write("tname 参数为空\n"); return false; }
    // 待清理的表的唯一键字段名
    GetXMLBuffer(xmlbuffer, "keycol", starg.keycol, 31);
    if (strlen(starg.keycol) == 0)
    { logfile.Write("keycol 参数为空\n"); return false; }
    // 需要清理数据的条件
    GetXMLBuffer(xmlbuffer, "where", starg.where, 1001);
    if (strlen(starg.where) == 0)
    { logfile.Write("where 参数为空\n"); return false; }
    // 程序的开始时间(小时)。一般选择在数据库负载压力比较小的时间段进行，必须夜晚等。如果该参数为空，则立即执行数据清理
    GetXMLBuffer(xmlbuffer, "starttime", starg.starttime, 40);
    // 程序心跳的超时时间
    GetXMLBuffer(xmlbuffer, "timeout", &starg.timeout);
    if (starg.timeout <= 0)
    { logfile.Write("timeout 参数为%d，不符合标准\n", starg.timeout); return false; }
    // 程序心跳的名称
    GetXMLBuffer(xmlbuffer, "pname", starg.pname, 51);
    if (strlen(starg.pname) == 0)
    { logfile.Write("pname 参数为空\n"); return false; }

    return true;
}

// 判断当前时间是否符合starttime的标准
bool _isstarttime()
{
    if (strlen(starg.starttime) != 0)
    {
        char time[3];
        memset(time, 0, sizeof(time));
        LocalTime(time, "hh24");
        if (strstr(starg.starttime, time) == 0) return false;
    }

    return true;
}

// 数据清理主函数
bool _deletetabledata()
{
    CTimer Timer;
    sqlstatement stmtsel(&conn1);
    char selkeycol[51];
    // 从表中查询出待删除列的唯一键
    stmtsel.prepare("select %s from %s %s", starg.keycol, starg.tname, starg.where);
    stmtsel.bindout(1, selkeycol, 50);

    char delwhere[2001];
    memset(delwhere, 0, sizeof(delwhere));
    char stemp[11];
    // 拼接绑定删除SQL语句 where 唯一键 in (...)
    for (int ii = 0; ii < MAXPARAMS; ii ++)
    {
        SPRINTF(stemp, sizeof(stemp), ":%d,", ii+1);
        strcat(delwhere, stemp);
    }
    delwhere[strlen(delwhere) - 1] = 0; // 删除最后的逗号

    sqlstatement stmtdel(&conn2);
    char delbindin[MAXPARAMS][51];
    // 准备删除数据的SQL，一次删除MAXPARAMS条记录
    stmtdel.prepare("delete from %s where %s in (%s)", starg.tname, starg.keycol, delwhere);
    for (int ii = 0; ii < MAXPARAMS; ii++)
        stmtdel.bindin(ii+1, delbindin[ii], 50);

    // 执行查询语句
    if (stmtsel.execute() != 0)
    { logfile.Write("清理数据失败\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message); return false; }

    int count = 0;
    memset(delbindin, 0, sizeof(delbindin));
    while (true)
    {
        memset(selkeycol, 0, sizeof(selkeycol));
        // 获取结果集
        if (stmtsel.next() != 0) break;

        strcpy(delbindin[count], selkeycol);
        count++;

        // 每MAXPARAMS条记录执行一次删除语句
        if (count == MAXPARAMS)
        {
            if (stmtdel.execute() != 0)
            { logfile.Write("清理数据失败\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message); return false; }
            count = 0;
            memset(delbindin, 0, sizeof(delbindin));
            PActive.UptATime();
        }
    }

    // 如果不足MAXPARAMS条记录，再执行一次删除
    if (count > 0)
    {
        if (stmtdel.execute() != 0)
        { logfile.Write("清理数据失败\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message); return false; }
        count = 0;
        memset(delbindin, 0, sizeof(delbindin));
        PActive.UptATime();
    }

    if (stmtsel.m_cda.rpc > 0)
    {
        logfile.Write("delete from %s %d rows %.2fsec. \n", starg.tname, stmtsel.m_cda.rpc, Timer.Elapsed());
    }

    return true;
}

// 程序的退出函数
void _exit(int sig)
{
    logfile.Write("接收到信号%d，程序退出\n", sig);

    exit(sig);
}