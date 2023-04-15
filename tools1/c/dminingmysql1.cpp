/* 
 * 本程序是数据中心的公共功能模块，用于从mysql数据库源表抽取数据，生成xml文件。
 */
#include "_public.h"
#include "_mysql.h"

struct st_arg
{
    char connstr[101];      // 数据库的连接参数，格式：ip,username,password,dbname,port
    char charaset[51];      // 数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况
    char selectsql[1024];   // 从数据源数据库抽取数据的SQL语句，注意：时间函数的百分号%需要四个，显示出来才有两个，被prepare之后将剩下一个
    char fieldstr[501];     // 抽取数据的SQL语句输出结果集字段名，中间用逗号分隔。将作为xml文件的字段名
    char fieldlen[501];     // 抽取数据的SQL语句输出结果集字段的长度，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应
    char bfilename[31];     // 输出xml文件名的前缀
    char efilename[31];     // 输出xml文件名的后缀
    char outpath[301];      // 输出xml文件存放的目录
    char starttime[51];     // 程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02和13时则运行，其他时间不运行。
    char incfield[31];      // 递增字段名，它必须是fieldstr中的字段名，并且只能是整形，一般为自增字段。如果incfield为空，表示不采用增量抽取方案
    char incfilename[301];  // 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重挖全部的数据
    int  timeout;           // 进程心跳的超时时间
    char pname[51];         // 进程名，建议采用"dminingmysql_后缀"的方式
} starg;

#define MAXFIELDCOUNT   100     // 结果集字段的最大数
// #define MAXFIELDLEN     500     // 结果集字段值的最大长度
int     MAXFIELDLEN = -1;       // 结果集字段值的最大长度，可以在_xmltoarg函数中动态的调节大小

char strfieldname[MAXFIELDCOUNT][31];   // 结果集字段名数据，从starg.fieldstr解析得到
int  ifieldlen[MAXFIELDCOUNT];          // 结果集字段的长度数组，从starg.fieldlen解析得到
int  ifieldcount;                       // strfieldname和ifieldlen数组中有效字段的个数
int  incfieldpos = -1;                  // 递增字段在结束集数组中的位置

CLogFile        logfile;
connection      conn;
sqlstatement    stmt;
CPActive        PActive;

void _help();
bool _xmltoarg(char *strxmlbuffer);
bool instarttime();
bool _dminingmysql();
bool _connmysql();
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
    
    // 判断当前时间是否在starttime所含时间上
    if (instarttime() == false) return 0;

    // 增加进程的心跳
    // PActive.AddPInfo(starg.timeout, starg.pname);
    PActive.AddPInfo(5000, starg.pname);

    if ( _dminingmysql() == false )
    { logfile.Write("上传文件失败！\n"); return -1; }

    return 0;
}

void _help()
{

    printf("\n");
    printf("Use: dminingmysql logfilename xmlbuffer\n\n");

    printf("/project/tools1/bin/procctl 3600 /project/tools1/bin/dminingmysql /log/idc/dminingmysql.log \"<connstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr><charaset>utf8</charaset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>10,30,30,10,10,10</fieldlen><bfilename>ZHOBTCODE</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><timeout>30</timeout><pname>dminingmysq1_ZHOBTMIND</pname>\"\n\n");
    printf("/project/tools1/bin/procctl 30 /project/tools1/bin/dminingmysql /log/idc/dminingmysql.log \"<connstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr><charaset>utf8</charaset><selectsql>select obtid,date_format(ddatetime,'%%%%Y-%%%%m-%%%%d %%%%H:%%%%m:%%%%s'),t,p,u,wd,wf,r,vis,keyid from t_zhobtmind where keyid>:1 and ddatetime>timestampadd(minute,-120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><starttime></starttime><incfield>keyid</incfield><incfilename>/idcdata/dmining/dminingmysql_ZHOBTMIND_HYCZ.list</incfilename><timeout>30</timeout><pname>dminingmysq1_ZHOBTMIND_HYCZ</pname>\"\n\n");

    printf("本程序是通用的功能模块，用于从mysql数据源表抽取数据，生成xml文件。\n");
    printf("logfilename 是本程序的运行日志文件。\n");
    printf("xmlbuffer   抽取数据源数据，生成xml文件所需参数：\n");
    printf("  connstr         数据库的连接参数，格式：ip,username,password,dbname,port\n");
    printf("  charaset        数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况\n");
    printf("  selectsql       从数据源数据库抽取数据的SQL语句，注意：时间函数的百分号%需要四个，显示出来才有两个，被prepare之后将剩下一个\n");
    printf("  fieldstr        抽取数据的SQL语句输出结果集字段名，中间用逗号分隔。将作为xml文件的字段名\n");
    printf("  fieldlen        抽取数据的SQL语句输出结果集字段的长度，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应\n");
    printf("  bfilename       输出xml文件名的前缀\n");
    printf("  efilename       输出xml文件名的后缀\n");
    printf("  outpath         输出xml文件存放的目录\n");
    printf("  starttime       程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02和13时则运行，其他时间不运行。"\
                              "如果starttime为空，那么starttime参数将失效，只要本程序启动就会执行数据抽取，为了减少数据源"\
                              "的压力，从数据库抽取数据的时候，一般在对方数据库最闲的时候时进行\n");
    printf("  incfield        递增字段名，它必须是fieldstr中的字段名，并且只能是整形，一般为自增字段。"\
                              "如果incfield为空，表示不采用增量抽取方案\n");
    printf("  incfilename     已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重挖全部的数据\n");
    printf("  timeout         心跳的超时时间，单位：秒\n");
    printf("  pname           进程名，建议采用\"dminingmysql_后缀\"的方式\n\n\n");
}

bool _xmltoarg(char *strxmlbuffer)
{

    memset(&starg, 0, sizeof(starg));

    GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);      // 数据库的连接参数，格式：ip,username,password,dbname,port
    if (strlen(starg.connstr) == 0)
    { logfile.Write("connstr值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "charaset", starg.charaset, 50);     // 数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况
    if (strlen(starg.charaset) == 0)
    { logfile.Write("charaset值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "selectsql", starg.selectsql, 1023); // 从数据源数据库抽取数据的SQL语句，注意：时间函数的百分号%需要四个，显示出来才有两个，被prepare之后将剩下一个
    if (strlen(starg.selectsql) == 0)
    { logfile.Write("selectsql值为空！\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer, "fieldstr", starg.fieldstr, 500);    // 抽取数据的SQL语句输出结果集字段名，中间用逗号分隔。将作为xml文件的字段名
    if (strlen(starg.fieldstr) == 0)
    { logfile.Write("fieldstr值为空！\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer, "fieldlen", starg.fieldlen, 500);    // 抽取数据的SQL语句输出结果集字段的长度，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应
    if (strlen(starg.fieldlen) == 0)
    { logfile.Write("fieldlen值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "bfilename", starg.bfilename, 30);   // 输出xml文件名的前缀
    if (strlen(starg.bfilename) == 0)
    { logfile.Write("bfilename值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "efilename", starg.efilename, 30);   // 输出xml文件名的后缀
    if (strlen(starg.efilename) == 0)
    { logfile.Write("efilename值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "outpath", starg.outpath, 300);      // 输出xml文件存放的目录
    if (strlen(starg.outpath) == 0)
    { logfile.Write("outpath值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "starttime", starg.starttime, 50);

    GetXMLBuffer(strxmlbuffer, "incfield", starg.incfield, 30); 
    
    GetXMLBuffer(strxmlbuffer, "incfilename", starg.incfilename, 300);
    
    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);          // 进程心跳的超时时间
    if (starg.timeout == 0)
    { logfile.Write("timeout值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);           // 进程名
    if (strlen(starg.pname) == 0)
    { logfile.Write("pname值为空！\n"); return false; }
    
    CCmdStr CmdStr;

    // 把starg.fieldlen解析到ifieldlen数组中
    CmdStr.SplitToCmd(starg.fieldlen, ",");
    if ((ifieldcount = CmdStr.CmdCount()) > MAXFIELDCOUNT)
    { logfile.Write("fieldlen的字段数太多，超出了最大限制%d\n", MAXFIELDCOUNT); return false; }
    
    for (int ii = 0; ii < ifieldcount; ii++)
    {
        CmdStr.GetValue(ii, &ifieldlen[ii]);
        // if (ifieldlen[ii] > MAXFIELDLEN) ifieldlen[ii] = MAXFIELDLEN;
        if (ifieldlen[ii] > MAXFIELDLEN) MAXFIELDLEN = ifieldlen[ii];
    }

    // 把starg.fieldstr解析到strfieldname数组中
    CmdStr.SplitToCmd(starg.fieldstr, ",");
    if (CmdStr.CmdCount() > MAXFIELDCOUNT)
    { logfile.Write("fieldstr的字段数太多，超出了最大限制%d\n", MAXFIELDCOUNT); return false; }

    for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
        CmdStr.GetValue(ii, strfieldname[ii], 30);

    // 判断strfieldname和ifieldlen的元素数量是否相等
    if (ifieldcount != CmdStr.CmdCount())
    { logfile.Write("strfieldname和ifieldlen的元素数量不相等\n"); return false; }

    // 查找自增字段在结果集中的位置
    if (strlen(starg.incfield) != 0)
    {
        logfile.Write("incfield = %s\n", starg.incfield);
        for (int ii = 0; ii < ifieldcount; ii++)
            if (strcmp(strfieldname[ii], starg.incfield) == 0) { incfieldpos = ii; break; }
        if (incfieldpos == -1)
        { logfile.Write("递增字段%s不在列表%s中\n", starg.incfield, starg.fieldstr); return false; }
    }
    return true;
}

bool instarttime()
{
    // 进程运行的时间区间，例如02、13，如果进程启动时，时间刚好为02或13时，则运行，其他时间不运行
    if (starg.starttime != 0)
    {
        // 获取当前时间
        char HH24[3];   memset(HH24, 0, sizeof(HH24));
        LocalTime(HH24, "hh24");

        // 判断starttime数组中的值是否包含当前时间
        if (strstr(HH24, starg.starttime) == 0)
            return false;
    }

    return true;
}

// 上传文件的功能函数
bool _dminingmysql()
{
    // 连接数据库获取数据
    if (_connmysql() == false)
        return false;

    // 生成xml文件

    return true;
}

bool _connmysql()
{
    // 连接数据库
    if (conn.connecttodb(starg.connstr, starg.charaset) != 0)
    { logfile.Write("数据库连接失败\n"); return false; }

    // 准备sql语句 绑定变量
    stmt.connect(&conn);
    stmt.prepare(starg.selectsql);
    char strfieldvalue[ifieldcount][MAXFIELDLEN+1];
    for (int ii = 1; ii < ifieldcount; ii++)
        stmt.bindout(ii, strfieldvalue[ii-1], ifieldlen[ii-1]);

    // 执行sql语句
    if (stmt.execute() != 0)
    { logfile.Write("stmt.execute() 失败\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return false; }

    while (true)
    {
        memset(strfieldvalue, 0, sizeof(strfieldvalue));
        
        // 从结果集(缓冲区)中取出数据
        if (stmt.next() != 0) break;

        for (int ii = 1; ii < ifieldcount; ii++)
            logfile.WriteEx("<%s>%s</%s>", strfieldname[ii-1], strfieldvalue[ii-1], strfieldname[ii-1]);
        logfile.WriteEx("</endl>\n");
    }

    return true;
}

void EXIT(int sig)
{
    printf("接收到%s信号，进程退出\n\n", sig);
    exit(0);
}