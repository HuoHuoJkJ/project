/*
 * execsql.cpp      这是一个工具程序，用于执行一个sql脚本文件
 */
#include "_public.h"
#include "_mysql.h"

CLogFile    logfile;
connection  conn;
CPActive    PActive;

void _help(void);
void _exit(int sig);

int main(int argc, char *argv[])
{
    if (argc != 5)
    { _help(); return -1; }

    // CloseIOAndSignal(true); signal(SIGINT, _exit); signal(SIGTERM, _exit);

    if (logfile.Open(argv[4], "a+") == false)
    { printf("打开日志文件(%s)失败\n", argv[4]); return -1; }

    PActive.AddPInfo(150, "execsql");

    if (conn.connecttodb(argv[2], argv[3], 1) != 0)
    { logfile.Write("连接数据库错误\n%s\n%s\n", argv[2], conn.m_cda.message); return -1; }
    logfile.Write("连接数据库成功(%s)\n", argv[2]);

    CFile File;

    // 打开sql文件
    if (File.Open(argv[1], "r") == false)
    { logfile.Write("打开文件(%s)失败\n", argv[1]); return -1; }

    char strBuffer[1024];
    // 读取一行sql语句，如果没有;则继续读取
    while(true)
    {
        memset(strBuffer, 0, sizeof(strBuffer));
        if (File.FFGETS(strBuffer, sizeof(strBuffer) - 1, ";") == false) break;

        // 删除sql语句最后的;
        char *pp = strstr(strBuffer, ";");
        if (pp == 0) continue;
        pp[0] = 0;

        logfile.WriteEx("%s\n", strBuffer);

        int iret = conn.execute(strBuffer);

        if (iret == 0)
            logfile.Write("sql语句执行成功(rpc=%d)\n", conn.m_cda.rpc);
        else
            logfile.Write("sql语句执行失败(message=%s)\n", conn.m_cda.message);
        PActive.UptATime();
    }

    return 0;
}

void _help(void)
{
    printf("\nUse:execsql sqlfile connstr charaset logfile\n");
    printf("Example:/project/tools1/bin/procctl 120 /project/tools1/bin/execsql /project/idc1/sql/cleardata.sql \"127.0.0.1,root,DYT.9525ing,TestDB,3306\" utf8 /log/tools/execsql.log\n\n");

    printf("这是一个工具程序，用于执行一个sql脚本文件\n");
    printf("sqlfile sql脚本文件名，每条sql语句可以多行书写，分号表示一条sql语句的结束，不支持注释\n");
    printf("connstr 数据库连接参数\n");
    printf("charset 数据库字符集\n");
    printf("logfile 本程序运行的日志文件名\n");
    printf("本程序由procctl程序每隔120秒调度一次\n");
}

void _exit(int sig)
{
    logfile.Write("进程(%d)退出，sig=%d", getpid(), sig);
    conn.disconnect();
    exit(0);
}