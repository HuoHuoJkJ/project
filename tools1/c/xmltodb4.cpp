/*
 * 本程序是数据中心的公共功能模块，用于从mysql数据入库的操作
 */
#include "_public.h"
#include "_mysql.h"

// 表的列（字段）信息的结构体
struct st_columns
{
	char colname[31];   // 列名，MySQL的表名最长可为64，但是为了兼容其他数据库，我们采用30个字符
	char datatype[31];  // 列的数据类型，只采用number date char 三大类，如果有其他的类型，转换为这三种类型，否则跳过它
	int  collen;        // 列的长度，number固定20，date固定19，char的长度由表结构决定
	int  pkseq;         // 如果列是主键的字段，存放主键字段的顺序，从1开始，不是主键取值0
};

class CTABCOLS
{
public:
	CTABCOLS();

	int m_allcount;     // 全部字段的个数
	int m_pkcount;      // 主键字段的个数

	vector<struct st_columns> m_vallcols;   // 存放全部字段信息的容器
	vector<struct st_columns> m_vpkcols;    // 存放主键字段信息的容器

	char m_allcols[3001];   // 全部的字段名列表，以字符串存放，中间用半角的逗号分隔
	char m_pkcols[301];     // 主键字段名列表，以字符串存放，中间用半角逗号分隔

	// 初始化成员变量
	void initdata();

	// 获取指定表的全部字段信息
	bool allcols(connection *conn, char *tablename);

	// 获取指定表的主键字段信息
	bool pkcols(connection *conn, char *tablename);
};

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

char strinsertsql[10241];   // insert sql语句的暂存区
char strupdatesql[10241];   // update sql语句的暂存区

CTABCOLS        TABCOLS;
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
void _crtSql();
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
			logfile.Write("处理文件(%s)中 ... ", Dir.m_FullFileName);
			int iret = _xmltodb(Dir.m_FullFileName, Dir.m_FileName);

			// 如果处理xml文件成功，备份xml文件，写日志
			if (iret == 0)
			{
				logfile.WriteEx("成功\n");
				// 把xml文件移动到备份目录当中，一般不会发生错误，如果发生了错误，很有可能是操作系统存储空间不足等极端的情况，不需要输出日志信息，直接退出进程
				if (_mvXmlfileToBakErr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpathbak) == false) return false;
			}

			// 如果失败，分为多种情况
			// 没有匹配的入库参数 || 待入库的表不存在
			if ((iret == 1) || (iret == 2))
			{
				if (iret == 1) logfile.WriteEx("失败  没有匹配的入库参数\n");
				if (iret == 2) logfile.WriteEx("失败  待入库的表不存在\n");
				// 把xml文件移动到备份目录当中，一般不会发生错误，如果发生了错误，很有可能是操作系统存储空间不足等极端的情况，不需要输出日志信息，直接退出进程
				if (_mvXmlfileToBakErr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpatherr) == false) return false;
			}

			// 数据库错误，函数返回，程序退出
			if (iret == 4)
			{ logfile.Write("失败  数据库错误\n"); return false; }
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

// 将容器vstxmltotable中的一个元素赋值给stxmltotable
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
	// 例如：从目录中读到了ZHOBTMIND_20230421203036_HYCZ_1.xml，而此时从xml配置文件加载到容器中的filename的内容是ZHOBTCODE_*.XML，这是正确的匹配
	if (_findXmlFromTable(filename) == false)
		return 1;

	// 从数据字典中读取 `读取待入库的表` 中的 字段名、字段长、字段类型等信息
	// 获取表的字段和主键信息，如果获取失败，应该是数据库的连接已失效
	// 在本程序运行的过程中，如果数据库出现异常，一定会在这里发生
	if (TABCOLS.allcols(&conn, stxmltotable.tname) == false) return 4;
	if (TABCOLS.pkcols(&conn, stxmltotable.tname)  == false) return 4;

	// 如果TABCOLS.m_allcount == 0，说明待入库的表不存在，返回2
	if (TABCOLS.m_allcount == 0) return 2;

	// 拼接更新update 或者 插入insert 的sql语句
	_crtSql();

	// 准备更新update 或者 插入insert 的sql语句，绑定输入变量

	// 在处理xml文件之前，如果vstxmltotable中的某个结构体中有execsql，则先执行它

	// 打开xml文件

	// while (true)
	// {
		// 读取xml文件内容

		// 解析xml 赋值给绑定的输入变量

		// 执行sql语句
	// }

	// 提交

	return 0;
}

void _crtSql()
{
	memset(strinsertsql, 0, sizeof(strinsertsql));
	memset(strupdatesql, 0, sizeof(strupdatesql));

	// 拼接insert的sql语句
	char strinsertcount[3001];
	char strinsertvalue[3001];

	// insert into %s(%s) values(%s)
	memset(strinsertcount, 0, sizeof(strinsertcount));
	memset(strinsertvalue, 0, sizeof(strinsertvalue));

	// 所有的字段名都存在 CTABCOLS.m_allcols中，但是我们不希望使用keyid 和 upttime这两个字段
	// 使用容器中存储的字段名
	int colseq = 1;
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
	{
		// 如果字段是keyid  upttime  就跳过它们
		if (strcmp(TABCOLS.m_vallcols[ii].colname, "upttime") == 0 ||
			strcmp(TABCOLS.m_vallcols[ii].colname, "keyid")   == 0 ) continue;

		// 进行字段名的拼接
		strcat(strinsertcount, TABCOLS.m_vallcols[ii].colname); strcat(strinsertcount, ",");

		// 进行待绑定数字:1,:2...的拼接，注意区分keyid 和 upttime的导致ii++的问题，使用一个新的计数器colseq来解决
		// 还有date字段的特殊: str_to_date( , )
		char strtemp[101];
		if (strcmp(TABCOLS.m_vallcols[ii].datatype, "date") != 0)
			SPRINTF(strtemp, sizeof(strtemp)-1, ":%d", colseq);
		else
			SPRINTF(strtemp, sizeof(strtemp)-1, "str_to_date(:%d,'%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", colseq);

		strcat(strinsertvalue, strtemp); strcat(strinsertvalue, ",");
		colseq++;
	}

	// 删除最后的逗号
	strinsertcount[strlen(strinsertcount)-1] = 0;
	strinsertvalue[strlen(strinsertvalue)-1] = 0;

	// 拼接出完整的insert语句
	SPRINTF(strinsertsql, sizeof(strinsertsql), "insert into %s(%s) values(%s)", stxmltotable.tname, strinsertcount, strinsertvalue);

	// logfile.WriteEx("\nstrinsertsql = %s\n", strinsertsql);

	// 判断是否需要进行更新 1-更新 2-不更新
	if (stxmltotable.uptbz != 1) return ;

	// 拼接出update sql 语句
	// update T_ZHOBTMIND1 set t=:1,p=:2,u=:3,wd=:4,wf=:5,r=:6,vis=:7,upttime=now(),mint=:8,minttime=str_to_date(:9,’%Y%m%d%H%i%s’) where obtid=:10 and ddatetime=str_to_date(:11,’%Y%m%d%H%i%s’)
	// 这里 不能更新主键，where 后面跟着 主键 进行选择

	// 先将中TABCOLS.m_vpkcols[1].pkseq 的值 赋值给TABCOLS.m_vallcols[ii].pkseq
	for (int ii = 0; ii < TABCOLS.m_vpkcols.size(); ii++)
		for (int hh = 0; hh < TABCOLS.m_vallcols.size(); hh++)
			if (strcmp(TABCOLS.m_vallcols[hh].colname, TABCOLS.m_vpkcols[ii].colname) == 0)
			{ TABCOLS.m_vallcols[hh].pkseq = TABCOLS.m_vpkcols[ii].pkseq; break; }

	// 拼接update语句 set 前面的部分
	SPRINTF(strupdatesql, sizeof(strupdatesql), "update %s set ", stxmltotable.tname);

	// 拼接update语句 set 后面的部分 到 where
	colseq = 1;
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
	{
		// 对字段keyid不做处理
		if (strcmp(TABCOLS.m_vallcols[ii].colname, "keyid") == 0) continue;

		// 判断是否为主键，如果是主键，则不在此处处理
		if (TABCOLS.m_vallcols[ii].pkseq != 0) continue;

		// 对upttime字段，直接使用数据库函数now()进行赋值处理
		if (strcmp(TABCOLS.m_vallcols[ii].colname, "upttime") == 0) { strcat(strupdatesql, "upttime=now(),"); continue; }

		// 判断date类型的字段，进行字符串转日期处理
		char strtemp[101];
		if (strcmp(TABCOLS.m_vallcols[ii].datatype, "date") != 0)
			SPRINTF(strtemp, sizeof(strtemp)-1,
					"%s=:%d", TABCOLS.m_vallcols[ii].colname, colseq);
		else
			SPRINTF(strtemp, sizeof(strtemp)-1,
					"%s=str_to_date(:%d,'%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", TABCOLS.m_vallcols[ii].colname, colseq);

		strcat(strupdatesql, strtemp); strcat(strupdatesql, ",");
		colseq++;
	}
	// 删除sql语句末尾的空格
	strupdatesql[strlen(strupdatesql)-1] = 0;


	// 拼接出where 部分， 1=1是恒等式，永远成立，这样做是为了后续拼接时方便
	strcat(strupdatesql, " where 1=1");

	// 拼接where后面的部分
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
	{
		if (TABCOLS.m_vallcols[ii].pkseq == 0) continue;

		char strtemp[101];
		if (strcmp(TABCOLS.m_vallcols[ii].datatype, "date") != 0)
			SPRINTF(strtemp, sizeof(strtemp)-1,
					"%s=:%d", TABCOLS.m_vallcols[ii].colname, colseq);
		else
			SPRINTF(strtemp, sizeof(strtemp)-1,
					"%s=str_to_date(:%d,'%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", TABCOLS.m_vallcols[ii].colname, colseq);
		strcat(strupdatesql, " and "); strcat(strupdatesql, strtemp);
		colseq++;
	}

	// logfile.WriteEx("\nstrupdatesql = %s\n", strupdatesql);
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

CTABCOLS::CTABCOLS()
{
	initdata();
}

void CTABCOLS::initdata()
{
	m_allcount = m_pkcount = 0;
	m_vallcols.clear();
	m_vpkcols.clear();
	memset(m_allcols, 0, sizeof(m_allcols));
	memset(m_pkcols, 0, sizeof(m_pkcols));
}

bool CTABCOLS::allcols(connection *conn, char *tablename)
{
	/*
	 * 使用sql语句
	 * select lower(column_name),lower(data_type),character_maximum_length from information_schema.COLUMNS
	 *  where table_name='T_ZHOBTMIND';
	 * 可以查询表T_ZHOBTMIND的列名、列的数据类型、所有列的最大字符长度（对于字符类型列）
	 *
	 * 在查询时，我们设置规范：表名使用大写，列名使用小写
	 * 因此在查询列名和列的数据类型时，我们使用了lower函数来将其转换为小写字母
	 */
	m_allcount = 0;
	m_vallcols.clear();
	memset(m_allcols, 0, sizeof(m_allcols));

	struct st_columns stcolumns;

	sqlstatement stmt;
	stmt.connect(conn);
	// 准备sql查询、绑定输入输出变量
	stmt.prepare("select lower(column_name),lower(data_type),character_maximum_length from information_schema.COLUMNS where table_name=:1");
	stmt.bindin(1, tablename, 30);
	stmt.bindout(1,  stcolumns.colname, 30);
	stmt.bindout(2,  stcolumns.datatype, 30);
	stmt.bindout(3, &stcolumns.collen);

	// 执行sql语句
	if (stmt.execute() != 0) return false;

	while (true)
	{
		memset(&stcolumns, 0, sizeof(struct st_columns));

		// 查询结果集
		if (stmt.next() != 0) break;

		// 处理列的数据类型。分为三大类number char date
		if (strcmp(stcolumns.datatype, "varchar") == 0)   strcpy(stcolumns.datatype, "char");
		if (strcmp(stcolumns.datatype, "char") == 0)      strcpy(stcolumns.datatype, "char");

		if (strcmp(stcolumns.datatype, "datetime") == 0)  strcpy(stcolumns.datatype, "date");
		if (strcmp(stcolumns.datatype, "timestamp") == 0) strcpy(stcolumns.datatype, "date");

		if (strcmp(stcolumns.datatype, "tintint") == 0)   strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "smallint") == 0)  strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "mediumint") == 0) strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "int") == 0)       strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "integer") == 0)   strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "bigint") == 0)    strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "numeric") == 0)   strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "decimal") == 0)   strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "float") == 0)     strcpy(stcolumns.datatype, "number");
		if (strcmp(stcolumns.datatype, "double") == 0)    strcpy(stcolumns.datatype, "number");

		// 如果业务有需要，可以修改上面的代码。增加对更多数据类型的支持
		// 如果字段中的数据类型不在上面列出来的中，忽略它。即对数据类型处理过后，不符合三大数据类型标准的，不做处理
		if (strcmp(stcolumns.datatype, "number") != 0 &&
			strcmp(stcolumns.datatype, "char")   != 0 &&
			strcmp(stcolumns.datatype, "date")   != 0 ) continue;

		// 设置字段的长度，number为20，date为19(yyyy-mm-dd hh:ii:ss)，char采用sql执行后的结果
		if (strcmp(stcolumns.datatype, "number") == 0) stcolumns.collen = 20;
		if (strcmp(stcolumns.datatype, "date") == 0)   stcolumns.collen = 19;

		strcat(m_allcols, stcolumns.colname);  strcat(m_allcols, ",");
		m_vallcols.push_back(stcolumns);
		m_allcount++;
	}

	// 删除最后的逗号","
	if (m_allcount > 0)
		m_allcols[strlen(m_allcols)-1] = 0;
		// strcat(&m_allcols[strlen(m_allcols)-1], "");

	return true;
}

bool CTABCOLS::pkcols(connection *conn, char *tablename)
{
	// 初始化成员变量
	m_pkcount = 0;
	m_vpkcols.clear();
	memset(m_pkcols, 0, sizeof(m_pkcols));
	struct st_columns stcolumns;

	/*
	 * select lower(column_name),seq_in_index from information_schema.STATISTICS
	 *  where table_name='T_ZHOBTMIND' and index_name='primary' order by seq_in_index
	 * 查询表中的主键，按照字段顺序排序
	 */
	sqlstatement stmt;
	stmt.connect(conn);
	// 准备sql语句
	stmt.prepare("select lower(column_name),seq_in_index from information_schema.STATISTICS where table_name=:1 and index_name='primary' order by seq_in_index");
	stmt.bindin(1, tablename, 30);
	stmt.bindout(1,  stcolumns.colname, 30);
	stmt.bindout(2, &stcolumns.pkseq);

	if (stmt.execute() != 0) return false;

	while (true)
	{
		memset(&stcolumns, 0, sizeof(struct st_columns));

		if (stmt.next() != 0) break;

		strcat(m_pkcols, stcolumns.colname); strcat(m_pkcols, ",");

		m_vpkcols.push_back(stcolumns);

		m_pkcount++;
	}

	if (m_pkcount > 0) m_pkcols[strlen(m_pkcols)-1] = 0;

	return true;
}