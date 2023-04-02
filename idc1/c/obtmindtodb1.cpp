/* 
 * obtmindtodb.cpp      本程序用于将站点测量的数据入库
 */
#include "_public.h"
#include "_mysql.h"

CLogFile    logfile;    // 日志文件类
connection  conn;       // 连接mysql类

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
    // CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT); // 调试进程阶段暂时不启用
    
    // 打开程序的日志文件
    if (logfile.Open(argv[4], "a+") == false)
    { printf("打开日志文件失败\n"); return -1; }
    
    // 获取目录下所有的站点观测数据文件，将其入库
    _obtmindtodb(argv[1], argv[2], argv[3]);

    // 连接数据库
    // if (conn.connecttodb(argv[2], argv[3]) != 0)
    // { logfile.Write("连接数据库失败\n%s\n%s",argv[2] , conn.m_cda.message); return -1; }


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
    CDir Dir;
    // 打开目录
    if (Dir.OpenDir(pathname, "*.xml") == false)
    { logfile.Write("打开目录(%s)失败\n", pathname); return false; }
    
    while (true)
    {
        // 读取目录，得到一个数据文件名
        if (Dir.ReadDir() == false) break;
        
        logfile.Write("filename=%s\n", Dir.m_FullFileName);
        
        // 打开文件
        /* 
        while (true)
        {
            // 处理文件中的每一行
        }
        */ 
        // 删除文件、提交事物
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