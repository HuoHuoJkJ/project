/* 
 * 本程序是数据中心的公共功能模块，用于从mysql数据入库的操作
 */
#include "_public.h"
#include "_mysql.h"

struct st_arg
{
    char connstr[101];      // 数据库的连接参数，格式：ip,username,password,dbname,port
    char charaset[51];      // 数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况
    char inifilename[301];  // 数据入库的参数配置文件
    char xmlpath[301];      // 待入库xml文件存放的目录
    char xmlpathbak[301];   // xml文件入库后的备份目录
    char xmlpatherr[301];   // 入库失败的xml文件存放的目录
    int  timetvl;           // 本程序运行的时间间隔，本程序常驻内存
    int  timeout;           // 本程序运行时的超时时间
    char pname[51];         // 本程序运行时的程序名
} starg;

struct st_xmltotable
{
    char filename[301];     // xml文件的匹配规则，用逗号分隔
    char tname[51];         // 待入库的表名
    int  uptbz;             // 更新标志：1-更新；2-不更新
    char execsql[301];      // 处理xml文件之前，执行SQL语句
} stxmltotable;
vector <struct st_xmltotable> vstxmltotable;

CLogFile        logfile;
connection      conn;
sqlstatement    stmt;
CPActive        PActive;

void _help();
bool _XmlToArg(char *strxmlbuffer);
bool _XmlToDB();
bool _loadXmlToTable();
bool _findXmlFromTable(const char *xmlfilename);
int  _xmltodb(char *fullname, char *filename);
bool _mvXmlfileToBakErr(char *filename, char *srcpath, char *destpath);
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
    if ( _XmlToArg(argv[2]) == false )
    { logfile.Write("解析xml失败！\n"); return -1; }
    
    // 增加进程的心跳
    // PActive.AddPInfo(starg.timeout, starg.pname);
    PActive.AddPInfo(5000, starg.pname);

    // 连接数据库
    if (conn.connecttodb(starg.connstr, starg.charaset) != 0)
    { logfile.Write("数据库连接失败\n"); return false; }
    logfile.Write("连接数据库%s成功\n", starg.connstr);
    
    if (_XmlToDB() == false)
        return -1;

    return 0;
}

void _help()
{

    printf("\n");
    printf("Use: xmltodb logfilename xmlbuffer\n\n");

    printf("/project/tools1/bin/procctl 10 /project/tools1/bin/xmltodb /log/idc/xmltodb_vip1.log \"<connstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr><charaset>utf8</charaset><inifilename>/project/tools/ini/xmltodb.xml</inifilename><xmlpath>/idcdata/xmltodb/vip1</xmlpath><xmlpathbak>/idcdata/xmltodb/vip1bak</xmlpathbak><xmlpatherr>/idcdata/xmltodb/vip1err</xmlpatherr><timetvl>5</timetvl><timeout>50</timeout><pname>xmltodb_vip1</pname>\"\n\n");

    printf("本程序是通用的功能模块，用于把xml文件入库到MySQL的表中。\n");
    printf("logfilename 是本程序的运行日志文件。\n");
    printf("xmlbuffer   抽取数据源数据，生成xml文件所需参数：\n");
    printf("  connstr         数据库的连接参数，格式：ip,username,password,dbname,port\n");
    printf("  charaset        数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况\n");
    printf("  inifilename     数据入库的参数配置文件\n");
    printf("  xmlpath         待入库xml文件存放的目录\n");
    printf("  xmlpathbak      xml文件入库后的备份目录\n");
    printf("  xmlpatherr      入库失败的xml文件存放的目录\n");
    printf("  timetvl         本程序的时间间隔，单位：秒，视业务需求而定，2-30之间。本程序常驻内存\n");
    printf("  timeout         心跳的超时时间，单位：秒，视xml文件大小而定，建议设置30秒以上，以免程序超时，被守护进程杀掉\n");
    printf("  pname           进程名，建议采用\"xmltodb_后缀\"的方式\n\n\n");
}

bool _XmlToArg(char *strxmlbuffer)
{
    memset(&starg, 0, sizeof(starg));

    GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);      // 数据库的连接参数，格式：ip,username,password,dbname,port
    if (strlen(starg.connstr) == 0)
    { logfile.Write("connstr值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "charaset", starg.charaset, 50);     // 数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况
    if (strlen(starg.charaset) == 0)
    { logfile.Write("charaset值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "inifilename", starg.inifilename, 300); // 数据入库的参数配置文件
    if (strlen(starg.inifilename) == 0)
    { logfile.Write("inifilename值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "xmlpath", starg.xmlpath, 300);      // 待入库xml文件存放的目录
    if (strlen(starg.xmlpath) == 0)
    { logfile.Write("xmlpath值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "xmlpathbak", starg.xmlpathbak, 300);// xml文件入库后的备份目录
    if (strlen(starg.xmlpathbak) == 0)
    { logfile.Write("xmlpathbak值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "xmlpatherr", starg.xmlpatherr, 300);// 入库失败的xml文件存放的目录
    if (strlen(starg.xmlpatherr) == 0)
    { logfile.Write("xmlpatherr值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);          // 本程序的时间间隔，单位：秒，视业务需求而定，2-30之间。本程序常驻内存
    if (starg.timetvl <= 2 && starg.timetvl >= 30)
    { logfile.Write("timetvl的值不规范，已改为30！\n"); starg.timetvl = 30; }

    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);          // 进程心跳的超时时间
    if (starg.timeout == 0)
    { logfile.Write("timeout值为空！\n"); return false; }

    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);           // 进程名
    if (strlen(starg.pname) == 0)
    { logfile.Write("pname值为空！\n"); return false; }

    return true;
}

bool _XmlToDB()
{
    CDir Dir;
    int count = 50; // 用来记录循环的次数，第一次的值设置为大于30，可以在程序首次运行时，加载xml配置文件到容器中，之后，每循环30次，就重新加载一次
                    // 虽然xml配置文件会进行改动，但是改动的频率很低，没有必要每次循环都进行加载

    while (true)
    {
        if (count++ > 30)
        {
            // 每执行一次加载...函数，便将计数器置为0
            count = 0;  
            // 把数据入库的参数配置文件inifilename加载到容器中
            if (_loadXmlToTable() == false) return false;
        }

        // 打开目录xmlpath，需要对其进行排序，使得先生成的文件先入库
        if (Dir.OpenDir(starg.xmlpath, "*.XML", 10000, false, true) == false)
        { logfile.Write("打开目录(%s)失败\n", starg.xmlpath); return false; }

        while (true)
        {
            // 读取目录，得到一个xml文件
            if (Dir.ReadDir() == false) break;

            // 处理xml内容
            // 调用文件处理子函数
            int iret = _xmltodb(Dir.m_FullFileName, Dir.m_FileName);

            logfile.Write("处理文件(%s)中 ... ", Dir.m_FullFileName);
            // 如果处理xml文件成功，备份xml文件，写日志
            if (iret == 0)
            {
                logfile.WriteEx("成功\n");
                // 把xml文件移动到备份目录当中，一般不会发生错误，如果发生了错误，很有可能是操作系统存储空间不足等极端的情况，不需要输出日志信息，直接退出进程
                if (_mvXmlfileToBakErr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpathbak) == false) return false;
            }

            // 如果失败，分为多种情况
            if (iret == 1)
            {
                logfile.WriteEx("失败  没有匹配的入库参数\n");
                // 把xml文件移动到备份目录当中，一般不会发生错误，如果发生了错误，很有可能是操作系统存储空间不足等极端的情况，不需要输出日志信息，直接退出进程
                if (_mvXmlfileToBakErr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpatherr) == false) return false;
            }
        }
        break;
        sleep(starg.timetvl);
    }

    return true;
}

bool _loadXmlToTable()
{
    CFile File;
    // 打开xml配置文件
    if (File.Open(starg.inifilename, "r") == false)
    { logfile.Write("打开xml配置文件(%s)失败\n", starg.inifilename); return false; }
    
    char buffer[501];

    while (true)
    {
        // 以<endl/>作为结束标志，读取一行内容，存入缓冲区中
        if (File.FFGETS(buffer, 500, "<endl/>") == false) break;

        memset(&stxmltotable, 0, sizeof(struct st_xmltotable));
        // 将缓冲区中的结果解析出来，赋值给stxmltotable
        GetXMLBuffer(buffer, "filename", stxmltotable.filename, 300);
        GetXMLBuffer(buffer, "tname", stxmltotable.tname, 50);
        GetXMLBuffer(buffer, "uptbz", &stxmltotable.uptbz);
        GetXMLBuffer(buffer, "execsql", stxmltotable.execsql, 300);
        
        // 加载到容器vstxmltotable中
        vstxmltotable.push_back(stxmltotable);
    }
    
    // 写入日志
    logfile.Write("加载xml配置文件(%s)内容到容器vstxmltotable成功\n", starg.inifilename);

    return true;
}

// 判断从目录中读取到的文件名的规则，是否符合xml配置文件中设置的filename的匹配规则
// 例如：从目录中读到了ZHOBTMIND_20230421203036_HYCZ_1.xml，而此时从xml配置文件加载到容器中的filename的内容是ZHOBTCODE_*.XML，这是正确的匹配
bool _findXmlFromTable(const char *xmlfilename)
{
    for (int ii = 0; ii < vstxmltotable.size(); ii++)
    {
        if (MatchStr(xmlfilename, vstxmltotable[ii].filename) == true)
        {
            memcpy(&stxmltotable, &vstxmltotable[ii], sizeof(struct st_xmltotable));

            return true;
        }
    }

    return false;
}

int  _xmltodb(char *fullname, char *filename)
{
    // 判断从目录中读取到的文件名的规则，是否符合xml配置文件中设置的filename的匹配规则。如果不符合，返回1
    if (_findXmlFromTable(filename) == false)
        return 1;

    return 0;
}

bool _mvXmlfileToBakErr(char *filename, char *srcpath, char *destpath)
{
    char filenamebak[301];
    STRCPY(filenamebak, sizeof(filenamebak), filename);
    
    UpdateStr(filenamebak, srcpath, destpath, false);
    
    if (RENAME(filename, filenamebak) == false)
    { logfile.Write("RENAME 文件 %s 到 %s 失败\n", filename, filenamebak); return false; }

    return true;
}

void EXIT(int sig)
{
    printf("接收到%s信号，进程退出\n\n", sig);
    exit(0);
}