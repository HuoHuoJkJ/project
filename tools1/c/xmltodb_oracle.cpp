/*
 * 本程序是数据中心的公共功能模块，用于从Oracle数据入库的操作
 */
#include "_tools_oracle.h"

/*
 * 与 xmltodb.cpp 程序的差异
 * 1. 修改程序的帮助文档
 * 2. 修改错误代码
 * 3. keyid字段的处理
 * 4. 时间函数和时间格式
 */

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

#define MAXCOLCOUNT 500 	// 表的最大字段的数量
// #define MAXCOLLEN   100		// 字段的存入的最大值
// char strcolvalue[MAXCOLCOUNT][MAXCOLLEN+1];	// 待绑定的输入变量
char *strcolvalue[MAXCOLCOUNT];	// 待绑定的输入变量

int  totalcount, inscount, uptcount;		// 记录总行数，插入行数，修改行数

CTABCOLS        TABCOLS;
CLogFile        logfile;
connection      conn;
sqlstatement    stmt, stmtins, stmtupt;
CPActive        PActive;

void _help();
bool _XmlToArg(char *strxmlbuffer);
bool _XmlToDB_oracle();
bool _loadXmlToTable();
bool _findXmlFromTable(const char *xmlfilename);
int  _xmltodb_oracle(char *fullname, char *filename);
void _crtSql();
void _prepareSql();
bool _execSql();
void _splitBuffer(char *buffer);
bool _mvXmlfileToBakErr(char *filename, char *srcpath, char *destpath);
void EXIT(int sig);

int main(int argc, char *argv[])
{
	// 写帮助文档
	if (argc != 3) { _help(); return -1; }

	// 处理程序的信号和IO
	CloseIOAndSignal(true); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);
	// 在整个程序编写完成且运行稳定后，关闭IO和信号。为了方便调试，暂时不启用CloseIOAndSignal();

	// 日志文件
	if ( logfile.Open(argv[1]) == false )
	{ printf("logfile.Open(%s) failed!\n", argv[1]); return -1; }

	// 解析xml字符串，获得参数
	if ( _XmlToArg(argv[2]) == false )
	{ logfile.Write("解析xml失败！\n"); return -1; }

	// 增加进程的心跳
	PActive.AddPInfo(starg.timeout, starg.pname);
	// PActive.AddPInfo(5000, starg.pname);

	if (_XmlToDB_oracle() == false) return -1;

	return 0;
}

void _help()
{

	printf("\n");
	printf("Use: xmltodb_oracle logfilename xmlbuffer\n\n");

	printf("/project/tools1/bin/procctl 10 /project/tools1/bin/xmltodb_oracle /log/idc/xmltodb_oracle_vip2.log \"<connstr>qxidc/dyting9525@snorcl11g_43</connstr><charaset>Simplified Chinese_China.AL32UTF8</charaset><inifilename>/project/tools/ini/xmltodb.xml</inifilename><xmlpath>/idcdata/xmltodb/vip2</xmlpath><xmlpathbak>/idcdata/xmltodb/vip2bak</xmlpathbak><xmlpatherr>/idcdata/xmltodb/vip2err</xmlpatherr><timetvl>5</timetvl><timeout>50</timeout><pname>xmltodb_oracle_vip2</pname>\"\n\n");

	printf("本程序是通用的功能模块，用于把xml文件入库到Oracle的表中。\n");
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
	printf("  pname           进程名，建议采用\"xmltodb_oracle_后缀\"的方式\n\n\n");
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

bool _XmlToDB_oracle()
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

			// 判读是否已经连接数据库，如果已经连接，就不需要再进行连接了
			// 这是为了多个进程运行时，防止重复连接数据库，提高效率
			if (conn.m_state == 0)
			{
				// 连接数据库
				if (conn.connecttodb(starg.connstr, starg.charaset) != 0)
				{ logfile.Write("数据库连接失败\n"); return false; }
				logfile.Write("连接数据库%s成功\n", starg.connstr);
			}

			// 处理xml内容
			// 调用文件处理子函数
			logfile.Write("处理文件(%s)中 ... ", Dir.m_FullFileName);
			int iret = _xmltodb_oracle(Dir.m_FullFileName, Dir.m_FileName);

			PActive.UptATime();

			// 如果处理xml文件成功，备份xml文件，写日志
			if (iret == 0)
			{
				logfile.WriteEx("成功(%s, total=%d, ins=%d, upt=%d)\n", stxmltotable.tname, totalcount, inscount, uptcount);
				// 把xml文件移动到备份目录当中，一般不会发生错误，如果发生了错误，很有可能是操作系统存储空间不足等极端的情况，不需要输出日志信息，直接退出进程
				if (_mvXmlfileToBakErr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpathbak) == false) return false;
			}

			// 如果失败，分为多种情况
			// 没有匹配的入库参数 || 待入库的表不存在
			if ((iret == 1) || (iret == 2) || (iret == 5))
			{
				if (iret == 1) logfile.WriteEx("失败  没有匹配的入库参数\n");
				if (iret == 2) logfile.WriteEx("失败  待入库的表(%s)不存在\n", stxmltotable.tname);
				if (iret == 5) logfile.WriteEx("失败  待入库的表(%s)的字段数量超出了程序所能处理的范围(%d)\n", stxmltotable.tname, MAXCOLCOUNT);
				// 把xml文件移动到备份目录当中，一般不会发生错误，如果发生了错误，很有可能是操作系统存储空间不足等极端的情况，不需要输出日志信息，直接退出进程
				if (_mvXmlfileToBakErr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpatherr) == false) return false;
			}

			// 打开XML文件失败，这种错误一般不会发生，如果真的发生了，那么一定是操作系统出现了问题，程序应当退出
			if (iret == 3)
			{ logfile.WriteEx("失败  打开xml文件失败\n"); return false; }

			// 数据库错误，函数返回，程序退出
			if (iret == 4)
			{ logfile.WriteEx("失败  数据库错误\n"); return false; }

			// 在处理xml文件之前，execsql执行失败
			if (iret == 6)
			{ logfile.WriteEx("失败  执行execsql语句失败\n"); return false; }
		}

		// 如果刚才扫描到了有文件，说明此时不空闲，可能不断的有文件生成，就不sleep了。
		// 是否扫描到了文件，可以由Dir类中的容器成员来判断
		if (Dir.m_vFileName.size() == 0) sleep(starg.timetvl);
		PActive.UptATime();
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

// xml站点数据和观测数据入库的子函数部分。
// 仅处理一个xml文件的入库操作
int  _xmltodb_oracle(char *fullname, char *filename)
{
	totalcount = inscount = uptcount = 0;
	// 判断从目录中读取到的文件名的规则，是否符合xml配置文件中设置的filename的匹配规则。如果不符合，返回1
	// 例如：从目录中读到了ZHOBTMIND_20230421203036_HYCZ_1.xml，而此时从xml配置文件加载到容器中的filename的内容是ZHOBTCODE_*.XML，这是正确的匹配
	if (_findXmlFromTable(filename) == false) return 1;

	// 清空上一次将xml文件入库后未释放的内存空间
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
		if (strcolvalue[ii] != 0) { delete strcolvalue[ii]; strcolvalue[ii] = 0; }

	// 从数据字典中读取 `读取待入库的表` 中的 字段名、字段长、字段类型等信息
	// 获取表的字段和主键信息，如果获取失败，应该是数据库的连接已失效
	// 在本程序运行的过程中，如果数据库出现异常，一定会在这里发生
	if (TABCOLS.allcols(&conn, stxmltotable.tname) == false) return 4;
	if (TABCOLS.pkcols(&conn, stxmltotable.tname)  == false) return 4;

	// 如果TABCOLS.m_allcount == 0，说明待入库的表不存在，返回2
	if (TABCOLS.m_allcount == 0) return 2;

	// 拼接更新update 或者 插入insert 的sql语句
	_crtSql();

	// 判断字段的数量是否超过了程序所能处理的最大数量
	if (TABCOLS.m_vallcols.size() > MAXCOLCOUNT) return 5;

	// 在此处动态的分配内存空间，以便绑定的变量能够存储待入库的数据
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
		strcolvalue[ii] = new char[TABCOLS.m_vallcols[ii].collen+1];

	// 准备更新update 或者 插入insert 的sql语句，绑定输入变量
	_prepareSql();

	// 在处理xml文件之前，如果vstxmltotable中的某个结构体中有execsql，则先执行它
	if (_execSql() == false) return 6;

	CFile File;
	// 打开xml文件
	if (File.Open(fullname, "r") == false)
	{ conn.rollback(); return 3; } // 回滚事务

	char strBuffer[10241];
	while (true)
	{
		// 读取xml文件内容
		if (File.FFGETS(strBuffer, sizeof(strBuffer)-1, "</endl>") == false) break;

		// 只要成功的读取了一行内容，就将总记录数++
		totalcount++;

		// 解析xml 赋值给绑定的输入变量
		_splitBuffer(strBuffer);

		// 执行sql语句
		if (stmtins.execute() != 0)
		{
			// 如果返回1062，说明表中已经存在待插入的数据，不能进行插入，进行更新操作
			if (stmtins.m_cda.rc == 1)
			{
				// 判断是否需要进行更新操作
				if (stxmltotable.uptbz == 1)
				{
					if (stmtupt.execute() != 0)
					{
						// 记录出错信息，但不终止程序
						logfile.Write("%s\n", strBuffer);
						logfile.Write("stmtupt.execute() 失败\n%s\n%s\n", stmtupt.m_sql, stmtupt.m_cda.message);

						// 数据库连接已失效，无法继续，只能返回。
						// 3113-在操作过程中服务器关闭。3114-查询过程中丢失了与Oracle服务器的连接。
						if ((stmtupt.m_cda.rc == 3113) || (stmtupt.m_cda.rc==3114)) return 4;
					}
					else uptcount++;
				}
			}
			else
			{
				// 记录出错信息，但不终止程序
				logfile.Write("%s\n", strBuffer);
				logfile.Write("stmtins.execute() 失败\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);

				// 数据库连接已失效，无法继续，只能返回。
				// 3113-在操作过程中服务器关闭。3114-查询过程中丢失了与Oracle服务器的连接。
				if ((stmtins.m_cda.rc == 3113) || (stmtins.m_cda.rc==3114)) return 4;
			}
		}
		else inscount++;
	}

	// 提交
	conn.commit();

	return 0;
}

void _crtSql()
{
	memset(strinsertsql, 0, sizeof(strinsertsql));
	memset(strupdatesql, 0, sizeof(strupdatesql));

	// 拼接insert的sql语句
	// insert into T_ZHOBTCODE1(obtid,cityname,provname,lat,lon,height) values(:1,:2,:3,:4,:5,:6)
	// insert into T_ZHOBTMIND1(obtid,ddatetime,t,p,u,wd,wf,r,vis,minttime) values(:1,to_date(:2,'yyyymmddhh24miss'),:3,:4,:5,:6,:7,:8,:9,to_date(:10,'yyyymmddhh24miss'))
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
		// 如果字段是upttime就跳过它
		if (strcmp(TABCOLS.m_vallcols[ii].colname, "upttime") == 0) continue;

		// 进行字段名的拼接
		strcat(strinsertcount, TABCOLS.m_vallcols[ii].colname); strcat(strinsertcount, ",");

		// 进行待绑定数字:1,:2...的拼接，注意区分upttime的导致ii++的问题，使用一个新的计数器colseq来解决
		// 还有date字段的特殊: to_date( , )
		char strtemp[101];
		// 如果是keyid字段
		if (strcmp(TABCOLS.m_vallcols[ii].colname, "keyid")  == 0)
			SPRINTF(strtemp, sizeof(strtemp)-1, "SEQ_%s.nextval", stxmltotable.tname+2);
		else
		{
			if (strcmp(TABCOLS.m_vallcols[ii].datatype, "date") != 0)
				SPRINTF(strtemp, sizeof(strtemp)-1, ":%d", colseq);
			else
				SPRINTF(strtemp, sizeof(strtemp)-1, "to_date(:%d,'yyyymmddhh24miss')", colseq);

			colseq++;
		}
		strcat(strinsertvalue, strtemp); strcat(strinsertvalue, ",");
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
	// update T_ZHOBTCODE1 set cityname=:1,provname=:2,lat=:3,lon=:4,height=:5,upttime=sysdate where 1=1 and obtid=:6
	// update T_ZHOBTMIND1 set t=:1,p=:2,u=:3,wd=:4,wf=:5,r=:6,vis=:7,upttime=sysdate,mint=:8,minttime=to_date(:9,’yyyymmddhh24miss’) where obtid=:10 and ddatetime=to_date(:11,’yyyymmddhh24miss’)
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

		// 对upttime字段，直接使用数据库函数sysdate进行赋值处理
		if (strcmp(TABCOLS.m_vallcols[ii].colname, "upttime") == 0) { strcat(strupdatesql, "upttime=sysdate,"); continue; }

		// 判断date类型的字段，进行字符串转日期处理
		char strtemp[101];
		if (strcmp(TABCOLS.m_vallcols[ii].datatype, "date") != 0)
			SPRINTF(strtemp, sizeof(strtemp)-1,
					"%s=:%d", TABCOLS.m_vallcols[ii].colname, colseq);
		else
			SPRINTF(strtemp, sizeof(strtemp)-1,
					"%s=to_date(:%d,'yyyymmddhh24miss')", TABCOLS.m_vallcols[ii].colname, colseq);

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
					"%s=to_date(:%d,'yyyymmddhh24miss')", TABCOLS.m_vallcols[ii].colname, colseq);
		strcat(strupdatesql, " and "); strcat(strupdatesql, strtemp);
		colseq++;
	}

	// logfile.WriteEx("\nstrupdatesql = %s\n", strupdatesql);
}

void _prepareSql()
{
	// 绑定insert 语句的输入变量
	// insert into T_ZHOBTCODE1(obtid,cityname,provname,lat,lon,height) values(:1,:2,:3,:4,:5,:6)
	//
	// insert into T_ZHOBTMIND1(obtid,ddatetime,t,p,u,wd,wf,r,vis,minttime)
	// 		values(:1,to_date(:2,'yyyymmddhh24miss'),:3,:4,:5,:6,:7,:8,:9,to_date(:10,'yyyymmddhh24miss'))
	stmtins.connect(&conn);
	stmtins.prepare(strinsertsql);
	// logfile.WriteEx("\n%s\n", strinsertsql);
	int colseq = 1;
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
	{
		// 忽略对keyid upttime
		if ((strcmp(TABCOLS.m_vallcols[ii].colname, "keyid")   == 0) ||
		    (strcmp(TABCOLS.m_vallcols[ii].colname, "upttime") == 0) ) continue;

		// 此处的ii可能并不连续，因为我们在上面忽略的对keyid upttime的处理
		stmtins.bindin(colseq, strcolvalue[ii], TABCOLS.m_vallcols[ii].collen);
		// logfile.WriteEx("bindin(%d, %s, %d)\n", colseq, TABCOLS.m_vallcols[ii].colname, TABCOLS.m_vallcols[ii].collen);
		colseq++;
	}

	// 判断是否需要进行update
	if (stxmltotable.uptbz != 1) return ;

	// 绑定update 语句的输入变量
	// update T_ZHOBTCODE1 set cityname=:1,provname=:2,lat=:3,lon=:4,height=:5,upttime=sysdate where 1=1 and obtid=:6
	// update T_ZHOBTMIND1 set t=:1,p=:2,u=:3,wd=:4,wf=:5,r=:6,vis=:7,upttime=sysdate,mint=:8,minttime=to_date(:9,’yyyymmddhh24miss’)
	// 		where obtid=:10 and ddatetime=to_date(:11,’yyyymmddhh24miss’)
	stmtupt.connect(&conn);
	stmtupt.prepare(strupdatesql);
	// logfile.WriteEx("\n%s\n", strupdatesql);

	// 在此处处理 set 到 where 之间的字段
	colseq = 1;
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
	{
		// 忽略对keyid upttime的处理
		if ((strcmp(TABCOLS.m_vallcols[ii].colname, "keyid")   == 0) ||
		    (strcmp(TABCOLS.m_vallcols[ii].colname, "upttime") == 0) ) continue;

		// 不在此处处理主键
		if (TABCOLS.m_vallcols[ii].pkseq != 0) continue;

		// 此处的ii可能并不连续，因为我们在上面忽略的对keyid upttime的处理
		stmtupt.bindin(colseq, strcolvalue[ii], TABCOLS.m_vallcols[ii].collen);
		// logfile.WriteEx("bindin(%d, %s, %d)\n", colseq, TABCOLS.m_vallcols[ii].colname,TABCOLS.m_vallcols[ii].collen);
		colseq++;
	}

	// 在此处处理 where 之后的字段（主键）
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
	{
		// 不在此处处理主键
		if (TABCOLS.m_vallcols[ii].pkseq == 0) continue;

		// 此处的ii可能并不连续，因为我们在上面忽略的对keyid upttime的处理
		stmtupt.bindin(colseq, strcolvalue[ii], TABCOLS.m_vallcols[ii].collen);
		// logfile.WriteEx("bindin(%d, %s, %d)\n", colseq, TABCOLS.m_vallcols[ii].colname, TABCOLS.m_vallcols[ii].collen);
		colseq++;
	}
}

// 在处理xml文件之前，如果vstxmltotable中的某个结构体中有execsql，则先执行它
bool _execSql()
{
	if (strlen(stxmltotable.execsql) == 0) return true;

	sqlstatement stmt;
	stmt.connect(&conn);
	stmt.prepare(stxmltotable.execsql);

	if (stmt.execute() != 0)
	{ logfile.Write("stmt.execute() 失败 \n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return false; }

	/*
	 * 这里即使成功了，也不能提交事务conn.commit(); 它的操作和将xml文件入库的操作需要放在同一个事务中提交
	 * 假设execsql的语句是delete表中数据
	 * 如果它执行成功了，但是后面将xml文件内容入库的操作失败了
	 * 新的数据插入失败了，旧的数据却又被删除了，导致数据库表成了空表
	 */

	return true;
}

// 解析buffer中的xml字符串，赋值给strcolvalue，以便后面进行数据入库的insert update操作
void _splitBuffer(char *buffer)
{
	/*
	 * 因为在_prepareSql函数中，我们使用字符串来绑定三大类(char number date)型的数据
	 * 所以在此处提取xml字符串时，无需考虑要赋值的变量的类型问题，因为变量都是char类型的
	 */

	// 初始化动态分配的内存空间
	// memset(strcolvalue, 0, sizeof(strcolvalue));
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
		memset(strcolvalue[ii], 0, TABCOLS.m_vallcols[ii].collen+1);

	char strtemp[31];

	// logfile.Write("\nbuffer = %s\n", buffer);
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
	{
		// 如果是date类型的，我们需要提取出字符串里面的数字，这是出于兼容性的考虑
		// 无论是怎么样的date格式，我们都将其转换成yyyymmddhh24iiss
		if (strcmp(TABCOLS.m_vallcols[ii].datatype, "date") == 0)
		{
			GetXMLBuffer(buffer, TABCOLS.m_vallcols[ii].colname, strtemp, TABCOLS.m_vallcols[ii].collen);
			PickNumber(strtemp, strcolvalue[ii], false, false);
			continue;
		}

		// 如果是数值字段，只提取数字、-、+符号和.
		if (strcmp(TABCOLS.m_vallcols[ii].datatype, "number") == 0)
		{
			GetXMLBuffer(buffer, TABCOLS.m_vallcols[ii].colname, strtemp, TABCOLS.m_vallcols[ii].collen);
			PickNumber(strtemp, strcolvalue[ii], true, true);
			continue;
		}

		// 如果是字符字段，直接提取
		GetXMLBuffer(buffer, TABCOLS.m_vallcols[ii].colname, strcolvalue[ii], TABCOLS.m_vallcols[ii].collen);
	}
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
	// 清空最后一次将xml文件入库后未释放的内存空间
	for (int ii = 0; ii < TABCOLS.m_vallcols.size(); ii++)
		if (strcolvalue[ii] != 0) { delete strcolvalue[ii]; strcolvalue[ii] = 0; }
	logfile.Write("接收到%d信号，进程退出\n\n", sig);

	exit(0);
}