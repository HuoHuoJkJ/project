/* 
 * obtmindtodb.cpp      本程序用于将站点测量的数据入库
 */
#include "idcapp.h"

CLogFile        logfile;        // 日志文件类
connection      conn;           // 连接mysql类
CPActive        PActive;        // 程序心跳类

// 帮助文档
void _help(void);
// 业务处理主函数
bool _obtmindtodb(const char *pathname, const char *connstr, const char *charaset);
// 程序退出
void EXIT(int sig);

int main(int argc, char *argv[])
{
    // 程序的帮助文档
    if (argc != 5)
    { _help(); return -1; }

    // 关闭进行的IO和信号
    CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT); // 调试进程阶段暂时不启用
    
    // 打开程序的日志文件
    if (logfile.Open(argv[4], "a+") == false) { printf("打开日志文件失败\n"); return -1; }
    
    // 进程心跳
    PActive.AddPInfo(20, "obtmindtodb");

    // 获取目录下所有的站点观测数据文件，将其入库
    _obtmindtodb(argv[1], argv[2], argv[3]);
    

    return 0;
}

// 帮助文档
void _help(void)
{
    printf("\nUse:obtmindtodb pathname connstr charaset logfile\n");
    printf("Example:/project/tools1/bin/procctl 10 /project/idc1/bin/obtmindtodb /idcdata/surfdata \"127.0.0.1,root,DYT.9525ing,TestDB,3306\" utf8 /log/idc/obtmindtodb.log\n\n");

    printf("本程序用于把全国站点分钟观测数据保存到数据库的T_ZHOBTMIND表中，数据只插入，不更新。\n");
    printf("pathname    全国站点分钟观测数据文件存放的目录。\n");
    printf("connstr     数据库连接参数：ip,username,password,dbname,port\n");
    printf("charaset    数据库的字符集\n");
    printf("logfile     本程序运行的日志文件名。\n");
    printf("程序每10秒运行一次，有procctl调度。\n\n\n");
}

// 业务处理主函数
bool _obtmindtodb(const char *pathname, const char *connstr, const char *charaset)
{
    CDir         Dir;            // 目录类
    CFile        File;           // 文件类
    CZHOBTMIND   ZHOBTMIND(&conn, &logfile);

    // 打开目录
    if (Dir.OpenDir(pathname, "*.xml") == false)
    { logfile.Write("打开目录(%s)失败\n", pathname); return false; }

    char tmp[11];   // 用于暂存结构体中需要进行小数转换为整数的变量的值
    
    int totalcount  = 0;    // 文件的总记录数
    int insertcount = 0;    // 成功插入记录数
    CTimer Timer;           // 计时器，记录每个数据的处理耗时

    while (true)
    {
        // 读取目录，得到一个数据文件名
        if (Dir.ReadDir() == false) break;

        // 打开文件
        if (File.Open(Dir.m_FullFileName, "r") == false)
        { logfile.Write("打开文件(%s)失败\n", Dir.m_FullFileName); return false; }

        // 判断是否已经连接。程序每十秒运行一次，如果每次运行都要连接一次数据库，开销是非常大的，因此做以下处理
        if (conn.m_state == 0)
        {
            // 连接数据库
            if (conn.connecttodb(connstr, charaset) != 0)
            { logfile.Write("连接数据库失败\n%s\n%s",connstr , conn.m_cda.message); return false; }
            logfile.Write("连接数据库成功(%s)\n", connstr);
        }

        char strBuffer[1001];
        while (true)
        {
            // 处理文件中的每一行
            if (File.FFGETS(strBuffer, sizeof(strBuffer) - 1, "<endl/>") == false) break;

            // 解析strBuffer中的xml字符串
            ZHOBTMIND.SplitBuffer(strBuffer);
            totalcount++;

            // 把结构体中的数据插入表中
            if (ZHOBTMIND.InsertTable() == true) insertcount++;
        }
        // 删除文件、提交事物
        // File.CloseAndRemove();
        conn.commit();

        // 总记录数，总插入数，插入一个文件耗时
        logfile.Write("文件(%s)----记录数：%d，插入数：%d，耗时：%lf\n", Dir.m_FileName, totalcount, insertcount, Timer.Elapsed());
        insertcount = totalcount = 0;
    }

    return true;
}

// 程序退出
void EXIT(int sig)
{
    printf("进程(%d)退出，sig=%d", getpid(), sig);
    
    conn.disconnect();

    exit(0);
}