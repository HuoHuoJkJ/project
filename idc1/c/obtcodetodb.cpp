/* 
 * obtcodetodb.cpp      本程序用于把全国站点参数数据保存到数据库表T_ZHOBTCODE中
 */
#include "_public.h"
#include "_mysql.h"

struct st_code                      // 与全国站点参数文件和数据库表的列一一对应的结构体
{
    char provname[31];
    char obtid[11];
    char cityname[31];
    char lat[11];
    char lon[11];
    char height[11];
};

vector<struct st_code> vstcode;     // 容器，用于存储全国站点参数，便于插入/更新到数据库表中

CLogFile    logfile;    // 日志文件类
connection  conn;       // 数据库连接类
CFile       File;       // 操作文件类
CPActive    PActive;    // 进程心跳类

// 程序帮助文档
void _help(void);
// 将站点参数添加到vstcode容器中
bool LoadToVst(const char *inifile);
// 进程退出
void EXIT(int sig);

int main(int argc, char *argv[])
{
    // 程序的帮助文档
    if (argc != 5)
    { _help(); return -1; }

    // 处理程序运行时的退出信号和IO
    CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    // 打开日志文件
    if (logfile.Open(argv[4], "a+") == false)
    { printf("打开日志文件失败\n"); return -1; }
    
    PActive.AddPInfo(10, "obtcodetodb");

    // 把全国站点参数文件加载到容器vstcode中
    if (LoadToVst(argv[1]) == false)
    { logfile.Write("将参数加载到vstcode失败\n"); return -1; }
    logfile.Write("加载参数文件(%s)成功，站点数(%d)\n", argv[1], vstcode.size());

    // 连接数据库 
    if (conn.connecttodb(argv[2], argv[3]) != 0)
    // if (conn.connecttodb("127.0.0.1,root,DYT.9525ing,TestDB,3306", "utf8") != 0)
    { logfile.Write("连接mysql失败，传入参数：%s，字符集：%s\n", argv[2], argv[3]); return -1; }
    logfile.Write("连接mysql成功(%s)\n", argv[2]);

    struct st_code stcode;
    // 准备插入表的SQL语句
    sqlstatement stmtins(&conn);
    stmtins.prepare(
                   "insert into T_ZHOBTCODE(obtid,cityname,provname,lat,lon,height,upttime) values(:1,:2,:3,:4*100,:5*100,:6*10,now())"
                   );
    stmtins.bindin(1, stcode.obtid, 10);
    stmtins.bindin(2, stcode.cityname, 30);
    stmtins.bindin(3, stcode.provname, 30);
    stmtins.bindin(4, stcode.lat, 10);
    stmtins.bindin(5, stcode.lon, 10);
    stmtins.bindin(6, stcode.height, 10);

    // 准备更新表的SQL语句
    sqlstatement stmtupt(&conn);
    stmtupt.prepare(
                   "update T_ZHOBTCODE set cityname=:1,provname=:2,lat=:3*100,lon=:4*100,height=:5*10,upttime=now() where obtid=:6"
                   );
    stmtupt.bindin(1, stcode.cityname, 30);
    stmtupt.bindin(2, stcode.provname, 30);
    stmtupt.bindin(3, stcode.lat, 10);
    stmtupt.bindin(4, stcode.lon, 10);
    stmtupt.bindin(5, stcode.height, 10);
    stmtupt.bindin(6, stcode.obtid, 10);

    int inscount = 0, uptcount = 0;
    CTimer Timer;

    // 遍历容器vstcode
    for (int ii = 0; ii < vstcode.size(); ii++)
    {
        // 从容器中取出一条记录到结构体stcode中(memcpy)
        memcpy(&stcode, &vstcode[ii], sizeof(struct st_code));

        // 执行插入的SQL语句
        if (stmtins.execute() != 0)
        {
            if (stmtins.m_cda.rc == 1062)
            {
                // 如果记录已存在，执行更新SQL语句
                if (stmtupt.execute() != 0)
                { logfile.Write("stmtupt.execute() 失败\n%s\n%s\n", stmtupt.m_sql, stmtupt.m_cda.message); return -1; }
                else
                    uptcount++;
            }
            else
            { logfile.Write("stmtins.execute() 失败\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message); return -1; }
        }
        else
            inscount++;
    }

    // 对表的修改进行总结。总修改、插入、更新
    logfile.Write("总记录数：%d， 插入：%d， 更新：%d， 时间：%d\n", vstcode.size(), inscount, uptcount, Timer.Elapsed());

    // 执行提交操作
    conn.commit();

    return 0;
}

// 程序帮助文档
void _help(void)
{
    printf("Use:    obtcodetodb inifile connstr charaset logfile\n");
    printf("Example:/project/tools1/bin/procctl 120 /project/idc1/bin/obtcodetodb /project/idc1/ini/stcode.ini \"127.0.0.1,root,DYT.9525ing,TestDB,3306\" utf8 /log/idc/obtcodetodb.log\n\n");
    
    printf("本程序用于把全国站点参数数据保存到数据库表T_ZHOBTCODE中\n");
    printf("inifile     存放全国站点参数的文件\n");
    printf("connstr     用于连接数据库的参数，如:\"192.168.0.1,username,password,mysqldb,3306\"\n");
    printf("charaset    设置数据库字符集\n");
    printf("logfile     程序的日志文件\n");
    printf("程序每120秒被进程procctl调度一次\n\n\n");
}

// 将站点参数添加到vstcode容器中
bool LoadToVst(const char *inifile)
{
    CCmdStr CmdStr;     // 分割字符串

    // 打开inifile文件(File.Open)
    if (File.Open(inifile, "r") == false)
    { logfile.Write("打开%s文件失败\n", inifile); return false; }

    char buffer[301];
    struct st_code stcode;

    while (true)
    {
        // memset(buffer, 0, sizeof(buffer));Fgets函数会初始化buffer
        // 逐行读取文件内容到buffer中(File.Fgets)
        if (File.Fgets(buffer, 300, true) == false) break;

        // 将buffer字符串以`,`为分隔符，把数据存放到stcode中(CmdStr.SplitToCmd)
        CmdStr.SplitToCmd(buffer, ",", true);
        // 由于文件的首行并没有使用`,`分隔，使用这种方法可以忽略首行写入vstcode
        if (CmdStr.CmdCount() != 6) continue;

        // 把分割出来的buffer字符串赋值给stcode
        memset(&stcode, 0, sizeof(struct st_code));
        CmdStr.GetValue(0, stcode.provname, 30);
        CmdStr.GetValue(1, stcode.obtid, 10);
        CmdStr.GetValue(2, stcode.cityname, 30);
        CmdStr.GetValue(3, stcode.lat, 10);
        CmdStr.GetValue(4, stcode.lon, 10);
        CmdStr.GetValue(5, stcode.height, 10);
        
        // 把站点参数stcode存入vstcode中
        vstcode.push_back(stcode);
    }
    
//   for (int ii=0;ii<vstcode.size();ii++)
    // logfile.Write("provname=%s,obtid=%s,cityname=%s,lat=%s,lon=%s,height=%s\n",\
                //    vstcode[ii].provname,vstcode[ii].obtid,vstcode[ii].cityname,vstcode[ii].lat,\
                //    vstcode[ii].lon,vstcode[ii].height);

    return true;
}

// 进程退出
void EXIT(int sig)
{
    logfile.Write("接收到%d信号，进程即将退出\n", sig);
    
    conn.disconnect();
    
    exit(0);
}