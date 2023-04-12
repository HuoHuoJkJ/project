/* 
 * 本程序是数据中心的公共功能模块，用于从mysql数据库源表抽取数据，生成xml文件。
 */
#include "_public.h"
#include "_mysql.h"

struct st_arg
{
    char connstr[101];      // 数据库的连接参数
    char charaset[51];      // 数据库的字符集
    char selectsql[1024];   // 从数据源数据库抽取数据的SQL语句
    char fieldstr[501];     // 抽取数据的SQL语句输出结果集字段名
    char fieldlen[501];     // 抽取数据的SQL语句输出结果集字段的长度
    char bfilename[31];     // 输出xml文件名的前缀
    char efilename[31];     // 输出xml文件名的后缀
    char outpath[301];      // 输出xml文件存放的目录
    char starttime[51];     // 程序运行的时间区间
    char incfield[31];      // 递增字段名
    char incfilename[301];  // 已抽取数据的递增字段最大值存放的文件
    int  timeout;           // 进程心跳的超时时间
    char pname[51];         // 进程名，建议采用"dminingmysql_后缀"的方式
} starg;

CLogFile logfile;
CPActive PActive;

void _help();
bool _xmltoarg(char *strxmlbuffer);
bool _dminingmysql();
void EXIT(int sig);

int main(int argc, char *argv[])
{
    // 写帮助文档
    if (argc != 3) { _help(); return -1; }

    // 处理程序的信号和IO
    CloseIOAndSignal(true);
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);
    // 在整个程序编写完成且运行稳定后，关闭IO和信号。为了方便调试，暂时不启用CloseIOAndSignal();

    // 日志文件
    if ( logfile.Open(argv[1]) == false )
    { printf("logfile.Open(%s) failed!\n", argv[1]); return -1; }

    // 解析xml字符串，获得参数
    if ( _xmltoarg(argv[2]) == false )
    { logfile.Write("解析xml失败！\n"); return -1; }
    
    PActive.AddPInfo(starg.timeout, starg.pname);

    if ( _dminingmysql() == false )
    { logfile.Write("上传文件失败！\n"); return -1; }

    return 0;
}

void _help()
{
    printf("本程序是通用的功能模块，用于从mysql数据源表抽取数据，生成xml文件。\n");
    printf("logfilename 是本程序的运行日志文件。\n");
    printf("xmlbuffer   抽取数据源数据，生成xml文件所需参数：\n");
    printf("  connstr         数据库的连接参数\n");
    printf("  charaset        数据库的字符集\n");
    printf("  selectsql       从数据源数据库抽取数据的SQL语句\n");
    printf("  fieldstr        抽取数据的SQL语句输出结果集字段名\n");
    printf("  fieldlen        抽取数据的SQL语句输出结果集字段的长度\n");
    printf("  bfilename       输出xml文件名的前缀\n");
    printf("  efilename       输出xml文件名的后缀\n");
    printf("  outpath         输出xml文件存放的目录\n");
    printf("  starttime       程序运行的时间区间\n");
    printf("  incfield        递增字段名\n");
    printf("  incfilename     已抽取数据的递增字段最大值存放的文件\n");
    printf("  timeout         心跳的超时时间\n");
    printf("  pname           进程名，建议采用\"dminingmysql_后缀\"的方式\n\n\n");

    printf("\n");
    printf("Use: dminingmysql logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools1/bin/procctl 30 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>lighthouse</username><password>dyt.9525</password><localpath>/tmp/idc/surfdata</localpath><remotepath>/tmp/ftpputest</remotepath><matchname>SURF_ZH*.JSON</matchname><ptype>1</ptype><localpathbak>/tmp/idc/surfdatabak</localpathbak><okfilename>/idcdata/ftplist/dminingmysql_surfdata.xml</okfilename><timeout>80</timeout><pname>dminingmysql_surfdata</pname>\"\n\n\n");
}

bool _xmltoarg(char *strxmlbuffer)
{
    memset(&starg, 0, sizeof(starg));
    GetXMLBuffer(strxmlbuffer, "host", starg.host, 30);              // 远程服务器的IP和端口。
    if (strlen(starg.host) == 0)
    { logfile.Write("host值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "mode", &starg.mode);                 // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
    if (starg.mode != 2) starg.mode = 1;

    GetXMLBuffer(strxmlbuffer, "username", starg.username, 30);      // 远程服务器的用户名。
    if (strlen(starg.username) == 0)
    { logfile.Write("username值为空！\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer, "password", starg.password, 30);      // 远程服务器的密码
    if (strlen(starg.password) == 0)
    { logfile.Write("password值为空！\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer, "localpath", starg.localpath, 300);   // 本地存放文件的目录
    if (strlen(starg.localpath) == 0)
    { logfile.Write("localpath值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "remotepath", starg.remotepath, 300);            // 远程服务器存放文件的目录
    if (strlen(starg.remotepath) == 0)
    { logfile.Write("remotepath值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname, 100);              // 待上传文件匹配的规则
    if (strlen(starg.matchname) == 0)
    { logfile.Write("matchname值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);                          // 文件上传成功后，本地文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) )
    { logfile.Write("ptype值不正确！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "localpathbak", starg.localpathbak, 300);      // 文件上传成功后，本地文件的备份目录，此参数只有当ptype=3时才有效。
    if ( (strlen(starg.localpathbak) == 0) && (starg.ptype == 3) )
    { logfile.Write("localpathbak值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "okfilename", starg.okfilename, 300);            // 已上传成功文件名清单，此参数只有当ptype=1时才有效。      
    if ( (strlen(starg.okfilename) == 0) && (starg.ptype == 1) )
    { logfile.Write("okfilename值为空！\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);                      // 进程心跳超时时间
    if (starg.timeout == 0)
    { logfile.Write("timeout值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);                       // 进程名
    if (strlen(starg.pname) == 0)
    { logfile.Write("pname值为空！\n"); return false; }

    return true;
}

// 上传文件的功能函数
bool _dminingmysql()
{
    
    return true;
}

void EXIT(int sig)
{
    printf("接收到%s信号，进程退出\n\n", sig);
    exit(0);
}