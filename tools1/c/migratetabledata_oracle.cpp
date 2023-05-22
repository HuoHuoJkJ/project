/*
 * 本程序是数据中心的公共功能模块，用于定时迁移数据
 */
#include "_tools_oracle.h"

#define MAXPARAMS 256

struct st_arg
{
    char connectstr[101];   // 数据库连接参数
    char srctname[51];      // 待迁移数据表的表名
    char dsttname[51];      // 迁移目的表的表名
    char keycol[31];        // 待迁移的表的唯一键字段名
    char where[1001];       // 需要迁移数据的条件
    char starttime[41];     // 程序的开始时间(小时)。一般选择在数据库负载压力比较小的时间段进行，必须夜晚等。如果该参数为空，则立即执行数据迁移
    int  maxcount;          // 分批迁移数据
    int  timeout;           // 程序心跳的超时时间
    char pname[51];         // 程序心跳的名称
} starg;

CLogFile    logfile;
CPActive    PActive;
connection  conn;

// 程序的帮助文档
void _help();
// 解析xmlbuffer参数
bool _xmltoarg(char *xmlbuffer);
// 判断当前时间是否符合starttime的标准
bool _isstarttime();
// 数据迁移主函数
bool _migratetabledata_oracle();
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

    if (conn.connecttodb(starg.connectstr, NULL) != 0)
    { logfile.Write("执行select语句的数据库连接失败(%s)\n%s\n", starg.connectstr, conn.m_cda.message); _exit(-1); }

    if (_migratetabledata_oracle() == false) _exit(-1);

    return 0;
}

// 程序的帮助文档
void _help()
{
    printf("\n");
    printf("Use:\n");
    printf("migratetabledata_oracle logfile xmlbuffer");
    printf("Example:\n");
    printf("/project/tools1/bin/procctl 3600 /project/tools1/bin/migratetabledata_oracle /log/idc/migratetabledata_oracle_T_ZHOBTMIND.log \"<connectstr>qxidc/dyting9525@snorcl11g_43</connectstr><srctname>T_ZHOBTMIND1</srctname><dsttname>T_ZHOBTMIND_HIS</dsttname><keycol>keyid</keycol><where>where ddatetime<sysdate-0.03</where><starttime>01,02,03,04,05,13</starttime><maxcount>300</maxcount><timeout>120</timeout><pname>migratetabledata_oracle_ZHOBTMIND1</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，用于定时迁移数据\n");

    printf("logfile     本程序用于记录日志的文件\n");
    printf("xmlbuffer   本程序的运行参数\n");

    printf("connectstr  数据库连接参数\n");
    printf("srctname    待迁移数据表的表名\n");
    printf("dsttname    迁移目的表的表名，注意，srctname和dsttname的结构必须完全相同\n");
    printf("keycol      待迁移的表的唯一键字段名\n");
    printf("where       需要迁移数据的条件\n");
    printf("starttime   程序的开始时间(小时)。一般选择在数据库负载压力比较小的时间段进行，必须夜晚等。如果该参数为空，则立即执行数据迁移\n");
    printf("maxcount    每执行一次迁移操作的记录数，不能超过MAXPARAMS(256)\n");
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

    // 待迁移数据表的表名
    GetXMLBuffer(xmlbuffer, "srctname", starg.srctname, 51);
    if (strlen(starg.srctname) == 0)
    { logfile.Write("srctname 参数为空\n"); return false; }

    // 迁移目的表的表名
    GetXMLBuffer(xmlbuffer, "dsttname", starg.dsttname, 51);
    if (strlen(starg.dsttname) == 0)
    { logfile.Write("dsttname 参数为空\n"); return false; }

    // 待迁移的表的唯一键字段名
    GetXMLBuffer(xmlbuffer, "keycol", starg.keycol, 31);
    if (strlen(starg.keycol) == 0)
    { logfile.Write("keycol 参数为空\n"); return false; }

    // 需要迁移数据的条件
    GetXMLBuffer(xmlbuffer, "where", starg.where, 1001);
    if (strlen(starg.where) == 0)
    { logfile.Write("where 参数为空\n"); return false; }

    // 程序的开始时间(小时)。一般选择在数据库负载压力比较小的时间段进行，必须夜晚等。如果该参数为空，则立即执行数据迁移
    GetXMLBuffer(xmlbuffer, "starttime", starg.starttime, 40);

    // 每执行一次迁移操作的记录数，不能超过MAXPARAMS(256)
    GetXMLBuffer(xmlbuffer, "maxcount", &starg.maxcount);
    if (starg.maxcount <= 0)
    { logfile.Write("maxcount 的值不合法(%d)\n", starg.maxcount); return false; }
    if (starg.maxcount > MAXPARAMS)
        starg.maxcount = MAXPARAMS;
    // { logfile.Write("maxcount 的值大于MAXPARAMS(%d)，已改为MAXPARAMS\n", starg.maxcount); starg.maxcount = MAXPARAMS; }

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

// 数据迁移主函数
bool _migratetabledata_oracle()
{
    CTimer Timer;

    CTABCOLS TABCOLS;
    // 数据迁移要执行插入的sql语句，所以要把表的全部字段名取出来
    if (TABCOLS.allcols(&conn, starg.dsttname) == false)
    {
        logfile.Write("取出表%s中全部的字段名失败\n", starg.dsttname);
        return false;
    }

    sqlstatement stmtsel(&conn);
    char selkeycol[51];     // 从数据源表查询到的待迁移记录的keyid字段的值
    // 从表中查询出待删除列的唯一键
    stmtsel.prepare("select %s from %s %s", starg.keycol, starg.srctname, starg.where);
    // logfile.Write("select %s from %s %s\n", starg.keycol, starg.srctname, starg.where);
    stmtsel.bindout(1, selkeycol, 50);

    char detwhere[2001];
    memset(detwhere, 0, sizeof(detwhere));
    char stemp[11];
    // 拼接绑定插入/删除SQL语句的 where 唯一键 in (...)
    for (int ii = 0; ii < starg.maxcount; ii ++)
    {
        SPRINTF(stemp, sizeof(stemp), ":%d,", ii+1);
        strcat(detwhere, stemp);
    }
    detwhere[strlen(detwhere) - 1] = 0; // 删除最后的逗号

    sqlstatement stmtins(&conn);
    // 准备插入到迁移目的表的sql
    // insert into dettname(cols) select %s from %s where keyid in (detwhere);
    stmtins.prepare("insert into %s(%s) select %s from %s where %s in (%s)",\
                     starg.dsttname, TABCOLS.m_allcols, TABCOLS.m_allcols, starg.srctname, starg.keycol, detwhere);

    sqlstatement stmtdel(&conn);
    char delbindin[starg.maxcount][51];
    // 准备删除数据的SQL，一次删除starg.maxcount条记录
    stmtdel.prepare("delete from %s where %s in (%s)", starg.srctname, starg.keycol, detwhere);
    // 将insert和delete语句一起进行绑定参数的操作
    for (int ii = 0; ii < starg.maxcount; ii++)
    {
        stmtins.bindin(ii+1, delbindin[ii], 50);
        stmtdel.bindin(ii+1, delbindin[ii], 50);
    }

    // 执行查询语句
    if (stmtsel.execute() != 0)
    { logfile.Write("stmtsel.execute()\n迁移数据失败\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message); return false; }

    int count = 0;
    memset(delbindin, 0, sizeof(delbindin));
    while (true)
    {
        memset(selkeycol, 0, sizeof(selkeycol));
        // 获取结果集
        if (stmtsel.next() != 0) break;

        strcpy(delbindin[count], selkeycol);
        count++;

        // 每starg.maxcount条记录执行一次删除语句
        if (count == starg.maxcount)
        {
            // 先执行插入，如果先删除，则insert中的select将查询不到任何内容
            if (stmtins.execute() != 0)
            {
                logfile.Write("stmtins.execute()\n迁移数据失败\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
                // 如果错误为唯一键冲突的代码，则不进行错误处理
                if (stmtins.m_cda.rc != 1) return false;
            }

            // 再执行删除
            if (stmtdel.execute() != 0)
            {
                logfile.Write("stmtdel.execute()\n迁移数据失败\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
                return false;
            }

            conn.commit();

            count = 0;
            memset(delbindin, 0, sizeof(delbindin));
            PActive.UptATime();
        }
    }

    // 如果不足starg.maxcount条记录，再执行一次删除
    if (count > 0)
    {
        // 先执行插入，如果先删除，则insert中的select将查询不到任何内容
        if (stmtins.execute() != 0)
        {
            logfile.Write("迁移数据失败\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
            // 如果错误为唯一键冲突的代码，则不进行错误处理
            if (stmtins.m_cda.rc != 1) return false;
        }

        // 再执行删除
        if (stmtdel.execute() != 0)
        {
            logfile.Write("迁移数据失败\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
            return false;
        }

        conn.commit();
    }

    if (stmtsel.m_cda.rpc > 0)
    {
        logfile.Write("migtate %s from %s %d rows %.2fsec. \n", starg.srctname, starg.dsttname, stmtsel.m_cda.rpc, Timer.Elapsed());
    }

    return true;
}

// 程序的退出函数
void _exit(int sig)
{
    logfile.Write("接收到信号%d，程序退出\n", sig);

    exit(sig);
}