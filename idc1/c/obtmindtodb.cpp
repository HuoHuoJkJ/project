/* 
 * obtmindtodb.cpp      本程序用于将站点测量的数据入库
 */
#include "_public.h"
#include "_mysql.h"

struct st_zhobtmind
{
    char obtid[11];
    char ddatetime[21];
    char t[11];
    char p[11];
    char u[11];
    char wd[11];
    char wf[11];
    char r[11];
    char vis[11];
} stzhobtmind;

CLogFile        logfile;        // 日志文件类
connection      conn;           // 连接mysql类

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
    sqlstatement stmt;    // sql语句类
    CDir         Dir;            // 目录类
    CFile        File;           // 文件类

    // 打开目录
    if (Dir.OpenDir(pathname, "*.xml") == false)
    { logfile.Write("打开目录(%s)失败\n", pathname); return false; }

    char tmp[11];   // 用于暂存结构体中需要进行小数转换为整数的变量的值

    while (true)
    {
        // 读取目录，得到一个数据文件名
        if (Dir.ReadDir() == false) break;

        // 判断是否已经连接。程序每十秒运行一次，如果每次运行都要连接一次数据库，开销是非常大的，因此做以下处理
        if (conn.m_state == 0)
        {
            // 连接数据库
            if (conn.connecttodb(connstr, charaset) != 0)
            { logfile.Write("连接数据库失败\n%s\n%s",connstr , conn.m_cda.message); return false; }
            logfile.Write("连接数据库成功(%s)\n", connstr);
        }

        if (stmt.m_state == 0)
        {
            stmt.connect(&conn);
            // 绑定sql变量与结构体变量
            stmt.prepare("insert into T_ZHOBTMIND(obtid,ddatetime,t,p,u,wd,wf,r,vis) values(:1,str_to_date(:2,'%%Y%%m%%d%%H%%i%%s'),:3,:4,:5,:6,:7,:8,:9)");
            stmt.bindin(1, stzhobtmind.obtid, 10);
            stmt.bindin(2, stzhobtmind.ddatetime, 14);
            stmt.bindin(3, stzhobtmind.t, 10);
            stmt.bindin(4, stzhobtmind.p, 10);
            stmt.bindin(5, stzhobtmind.u, 10);
            stmt.bindin(6, stzhobtmind.wd, 10);
            stmt.bindin(7, stzhobtmind.wf, 10);
            stmt.bindin(8, stzhobtmind.r, 10);
            stmt.bindin(9, stzhobtmind.vis, 10);
        }

        logfile.Write("filename=%s\n", Dir.m_FullFileName);
        
        // 打开文件
        if (File.Open(Dir.m_FullFileName, "r") == false)
        { logfile.Write("打开文件(%s)失败\n", Dir.m_FullFileName); return false; }

        char strBuffer[1001];
        while (true)
        {
            // 处理文件中的每一行
            if (File.FFGETS(strBuffer, sizeof(strBuffer) - 1, "<endl/>") == false) break;
            // logfile.Write("%s", strBuffer);

            // 解析strBuffer中的xml字符串
            memset(&stzhobtmind, 0, sizeof(stzhobtmind));
            GetXMLBuffer(strBuffer, "obtid", stzhobtmind.obtid, 10);
            GetXMLBuffer(strBuffer, "ddatetime", stzhobtmind.ddatetime, 14);
            GetXMLBuffer(strBuffer, "t", tmp, 10);          if (strlen(tmp)>0) snprintf(stzhobtmind.t, 10, "%d", (int)(atof(tmp)*10));
            GetXMLBuffer(strBuffer, "p", tmp, 10);          if (strlen(tmp)>0) snprintf(stzhobtmind.p, 10, "%d", (int)(atof(tmp)*10));
            GetXMLBuffer(strBuffer, "u", stzhobtmind.u, 10);
            GetXMLBuffer(strBuffer, "wd", stzhobtmind.wd, 10);
            GetXMLBuffer(strBuffer, "wf", tmp, 10);         if (strlen(tmp)>0) snprintf(stzhobtmind.wf, 10, "%d", (int)(atof(tmp)*10));
            GetXMLBuffer(strBuffer, "r", tmp, 10);          if (strlen(tmp)>0) snprintf(stzhobtmind.r, 10, "%d", (int)(atof(tmp)*10));
            GetXMLBuffer(strBuffer, "vis", tmp, 10);        if (strlen(tmp)>0) snprintf(stzhobtmind.vis, 10, "%d", (int)(atof(tmp)*10));

            // 把解析出的结构体变量写日志，查看有没有问题
            // logfile.Write("obtid=%s, ddatetime=%s, t=%s, p=%s, u=%s, wd=%s, wf=%s, r=%s, vis=%s\n", stzhobtmind.obtid, stzhobtmind.ddatetime, stzhobtmind.t, stzhobtmind.p, stzhobtmind.u, stzhobtmind.wd, stzhobtmind.wf, stzhobtmind.r, stzhobtmind.vis);
            // logfile.Write("%s,%s,%s,%s,%s,%s,%s,%s,%s\n", stzhobtmind.obtid, stzhobtmind.ddatetime, stzhobtmind.t, stzhobtmind.p, stzhobtmind.u, stzhobtmind.wd, stzhobtmind.wf, stzhobtmind.r, stzhobtmind.vis);
            // return true;
            
            // 把结构体中的数据插入表中
            if (stmt.execute() != 0)
            {
                /* 
                 * 1、失败的情况有哪些？是否全部的失败都要写日志？
                 * 答：失败的原因主要有二：一是重复记录；而是数据内容非法。
                 * 2、如果失败了怎么办？程序是否需要继续？是否rollbak回滚？是否返回false？
                 * 答：如果失败的原因是数据内容非法，记录日志后继续；如果记录丑哦那功夫，不必记录日志，直接继续。
                 */
                if (stmt.m_cda.rc != 1062)
                {
                    logfile.Write("Buffer=%s", strBuffer);
                    logfile.Write("stmt.execute() 失败\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
                }
            }
        }
        // 删除文件、提交事物
        // File.CloseAndRemove();
        conn.commit();
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