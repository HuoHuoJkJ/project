/*
 * 本程序是数据中心的公共功能模块，采用增量的方法同步MySQL数据库之间的表。
 * 注意：本程序不使用Federated引擎
 */
#include "_tools.h"

struct st_arg
{
	char localconnstr[101];     // 本地数据库的连接参数
	char charset[51];           // 数据库的字符集
	char localtname[31];        // 本地表名
	char remotecols[1001];      // 远程表的字段列表
	char localcols[1001];       // 本地表的字段列表
	char where[1001];           // 同步数据的条件
	char remoteconnstr[101];    // 远程数据库的连接参数
	char remotetname[31];       // 远程表名
	char remotekeycol[31];      // 远程表的键值字段名
	char localkeycol[31];       // 本地表的键值字段名
	int  timetvl;				// 增量同步的执行时间间隔
	int  timeout;               // 本程序运行时的超时时间
	char pname[51];             // 本程序运行时的程序名
} starg;

CLogFile logfile;
CPActive PActive;
CTABCOLS TABCOLS;
connection connloc;             // 本地数据库连接
connection connrem;             // 远程数据库连接

// 存放本地表的自增字段的最大值
long maxkeyvalue = 0;

// 程序的帮助文档
void _help();
// 把xml解析到参数starg结构体中
bool _xmltoarg(char *strxmlbuffer);
// 业务处理主函数
bool _syncincrementex(bool &bcontinue);
// 从本地表starg.localtname中，获取自增字段的最大值，存放在全局变量maxkeyvalue中
bool _findlocalmaxkey();
// 程序推出函数
void _exit(int sig);

int main(int argc, char *argv[])
{
	if (argc != 3) { _help(); return -1; }

	// 关闭信号和IO
	CloseIOAndSignal(true); signal(SIGINT, _exit); signal(SIGTERM, _exit);

	if (logfile.Open(argv[1],"a+") == false)
	{ printf("打开日志文件失败（%s）。\n", argv[1]); return -1; }

	// 把xml解析到参数starg结构中
	if (_xmltoarg(argv[2]) == false) return -1;

	// 当程序稳定了之后再启用，否则，调试程序时，可能会被守护进程误杀
	// PActive.AddPInfo(starg.timeout, starg.pname);

	if (connloc.connecttodb(starg.localconnstr, starg.charset) != 0)
	{ logfile.Write("connect database(%s) 失败\n%s\n", starg.localconnstr, connloc.m_cda.message); _exit(-1); }
	// logfile.Write("connect database(%s) 成功\n", starg.localconnstr);

	// 无论 remotecols 或者 localcols 是否为空， 那么我们需要使用 localtname表中的字段来填充它们
	// 获取 localtname 表的所有字段名，存储在类的 m_allcols
	// 全部的字段名列表，以字符串存放，中间用半角的逗号分隔
	TABCOLS.allcols(&connloc, starg.localtname);
	if (strlen(starg.remotecols) == 0) strcpy(starg.remotecols, TABCOLS.m_allcols);
	if (strlen(starg.localcols)  == 0) strcpy(starg.localcols,  TABCOLS.m_allcols);

	// 连接远程数据库
	if (connrem.connecttodb(starg.remoteconnstr, starg.charset) != 0)
	{ logfile.Write("连接远程数据库(%s)失败\n%s\n", starg.remoteconnstr, connrem.m_cda.message); return false; }
	// logfile.Write("connect database(%s) 成功\n", starg.remoteconnstr);

	bool bcontinue;
	while (true)
	{
		// 业务处理主函数
		if (_syncincrementex(bcontinue) == false) _exit(0);
		if (bcontinue == false) sleep(starg.timetvl);
		PActive.UptATime();
	}

	return 0;
}

// 程序的帮助文档
void _help()
{
	printf("Use:syncincrementex logfilename xmlbuffer\n\n");
	printf("Sample:\n");

	printf("/project/tools1/bin/procctl 10 /project/tools1/bin/syncincrementex /log/idc/syncincrementex_ZHOBTMIND2.log \"<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><remoteconnstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</remoteconnstr><charset>utf8</charset><remotetname>T_ZHOBTMIND1</remotetname><localtname>T_ZHOBTMIND2</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrementex1_ZHOBTMIND2</pname>\"\n\n");

	printf("/project/tools1/bin/procctl 10 /project/tools1/bin/syncincrementex /log/idc/syncincrementex_ZHOBTMIND3.log \"<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><remoteconnstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</remoteconnstr><charset>utf8</charset><remotetname>T_ZHOBTMIND1</remotetname><localtname>T_ZHOBTMIND3</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><where>and obtid like '54%%'</where><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrementex1_ZHOBTMIND2</pname>\"\n\n");

	printf("本程序是数据中心的公共功能模块，采用增量的方法同步mysql数据库之间的表。\n");

	printf("logfilename     本程序运行的日志文件。\n");
	printf("xmlbuffer       本程序运行的参数，用xml表示，具体如下：\n\n");


	printf("localconnstr    本地数据库的连接参数，格式：ip,username,password,dbname,port。\n");
	printf("charset         数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。\n");

	printf("localtname      本地表名。\n");

	printf("remotecols      远程表的字段列表，用于填充在select和from之间，所以，remotecols可以是真实的字段，也可以是函数的返回值或者运算结果。\n"\
		   "                如果本参数为空，就用localtname表的字段列表填充。\n");
	printf("localcols       本地表的字段列表，与remotecols不同，它必须是真实存在的字段。如果本参数为空，就用localtname表的字段列表填充。\n");

	printf("where           同步数据的条件，填充在select remotekeycol from remotetname where remotekeycol>:1之后。注意，不要加where关键字。\n");

	printf("remoteconnstr   远程数据库的连接参数，格式与localconnstr相同。\n");
	printf("remotetname     远程表名。\n");
	printf("remotekeycol    远程表的键值字段名，必须是唯一的。\n");
	printf("localkeycol     本地表的键值字段名，必须是唯一的。\n");

	printf("timetvl			执行同步的时间间隔，单位：秒；取值1-30。\n");
	printf("timeout         本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。\n");
	printf("pname           本程序运行时的进程名，尽可能采用易懂的、与其他进程不同的名称，方便排查故障。\n\n\n");
}

// 把xml解析到参数starg结构体中
bool _xmltoarg(char *strxmlbuffer)
{
	memset(&starg, 0, sizeof(struct st_arg));

	// 本地数据库的连接参数，格式：ip,username,password,dbname,port。
	GetXMLBuffer(strxmlbuffer, "localconnstr", starg.localconnstr, 100);
	if (strlen(starg.localconnstr) == 0)
	{ logfile.Write("localconnstr 为空。\n"); return false; }

	// 数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。
	GetXMLBuffer(strxmlbuffer, "charset", starg.charset, 50);
	if (strlen(starg.charset) == 0)
	{ logfile.Write("charset 为空。\n"); return false; }

	// 本地表名。
	GetXMLBuffer(strxmlbuffer, "localtname", starg.localtname, 30);
	if (strlen(starg.localtname) == 0)
	{ logfile.Write("localtname 为空。\n"); return false; }

	// 远程表的字段列表，用于填充在select和from之间，所以，remotecols可以是真实的字段，也可以是函数的返回值或者运算结果。如果本参数为空，就用localtname表的字段列表填充。
	GetXMLBuffer(strxmlbuffer, "remotecols", starg.remotecols, 1000);

	// 本地表的字段列表，与remotecols不同，它必须是真实存在的字段。如果本参数为空，就用localtname表的字段列表填充。
	GetXMLBuffer(strxmlbuffer, "localcols", starg.localcols, 1000);

	// 同步数据的条件，填充在select remotekeycol from remotetname where remotekeycol>:1之后。注意，不要加where关键字。
	GetXMLBuffer(strxmlbuffer, "where", starg.where, 1000);

	// 远程数据库的连接参数，格式与localconnstr相同。
	GetXMLBuffer(strxmlbuffer, "remoteconnstr", starg.remoteconnstr, 100);
	if (strlen(starg.remoteconnstr) == 0)
	{ logfile.Write("remoteconnstr 为空。\n"); return false; }

	// 远程表名。
	GetXMLBuffer(strxmlbuffer, "remotetname", starg.remotetname, 30);
	if (strlen(starg.remotetname) == 0)
	{ logfile.Write("remotetname 为空。\n"); return false; }

	// 远程表的键值字段名，必须是唯一的。
	GetXMLBuffer(strxmlbuffer, "remotekeycol", starg.remotekeycol, 30);
	if (strlen(starg.remotekeycol) == 0)
	{ logfile.Write("remotekeycol 为空。\n"); return false; }

	// 本地表的键值字段名，必须是唯一的。
	GetXMLBuffer(strxmlbuffer, "localkeycol", starg.localkeycol, 30);
	if (strlen(starg.localkeycol) == 0)
	{ logfile.Write("localkeycol 为空。\n"); return false; }

	GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
	if (starg.timetvl == 0)
	{ logfile.Write("timetvl 为空。\n"); return false; }
	if (starg.timetvl > 30)
	{ logfile.Write("timetvl 大于30，已改为30。\n"); starg.timetvl = 30; }

	// 本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。
	GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
	if (starg.timeout == 0)
	{ logfile.Write("timeout 为空。\n"); return false; }

	if (starg.timeout < starg.timetvl+10)
	{
		logfile.Write("starg.timeout < starg.timetvl+10，可能会造成超时误杀的情况，已将timeout(%d)改为timetvl+10(%d)\n", starg.timeout, starg.timetvl);
		starg.timeout = starg.timetvl + 10;
	}

	// 本程序运行时的进程名，尽可能采用易懂的、与其他进程不同的名称，方便排查故障。
	GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
	if (strlen(starg.pname) == 0)
	{ logfile.Write("pname为空。\n"); return false; }

	return true;
}

// 业务处理主函数
bool _syncincrementex(bool &bcontinue)
{
	bcontinue = false;
	CTimer Timer;

	// 从本地表starg.localtname中，获取自增字段的最大值，存放在全局变量maxkeyvalue中
	if (_findlocalmaxkey() == false) return false;

	// 获取本地表中字段的数量，便于下面创建remcolvalues
	CCmdStr CmdStr;
	CmdStr.SplitToCmd(starg.localcols, ",");
	int colcount = CmdStr.CmdCount();

	// 从远程表查找大于maxkeyvalue的字段，存储在remcolvalues中
	char remcolvalues[colcount][TABCOLS.m_maxcollen+1];
	sqlstatement stmtsel(&connrem);
	// 从远程数据库中查找需要同步记录的key字段的值
	stmtsel.prepare("select %s from %s where %s>:1 %s order by %s", starg.remotecols, starg.remotetname, starg.remotekeycol, starg.where, starg.remotekeycol);
	stmtsel.bindin(1, &maxkeyvalue);
	for (int ii = 0; ii < colcount; ii++)
		stmtsel.bindout(ii+1, remcolvalues[ii], TABCOLS.m_maxcollen);

	if (stmtsel.execute() != 0)
	{ logfile.Write("stmtsel 执行失败\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message); return false; }

	// 拼接SQL语句绑定参数的字符串，insert ... into localtname values(:1,:2,...:colcount);
	char strbind[2001];		// 绑定同步SQL语句参数的字符串
	memset(strbind, 0, sizeof(strbind));
	char strtemp[11];
	for (int ii = 0; ii < colcount; ii++)
	{
		SPRINTF(strtemp, sizeof(strtemp), ":%d,", ii+1);
		strcat(strbind, strtemp);
	}
	strbind[strlen(strbind)-1] = 0; // 删除末尾多余的逗号","

	sqlstatement stmtins(&connloc);     // 执行向本地表插入数据的SQL语句
	// 准备插入本地表数据的SQL语句。执行一次，插入一条数据
	// insert into localtname(localcols) values(:1,:2,...,:colcount);
	stmtins.prepare("insert into %s(%s) values(%s)", starg.localtname, starg.localcols, strbind);
	for (int ii = 0; ii < colcount; ii++)
		stmtins.bindin(ii+1, remcolvalues[ii], TABCOLS.m_maxcollen);
	// test
	// logfile.WriteEx("stmtins = insert into %s(%s) select %s from %s where %s in (%s)", starg.localtname, starg.localcols, starg.remotecols, starg.fedtname, starg.remotekeycol, strbind);

	while (true)
	{
		memset(remcolvalues, 0, sizeof(remcolvalues));
		// 获取需要同步数据的结果集
		if (stmtsel.next() != 0) break;

		// 向本地表中插入记录
		if (stmtins.execute() != 0)
		// 执行从本地表中插入记录，一般不会报错
		// 如果报错，肯定是数据库的问题或同步参数配置不正确（如：表名写错等），流程不必继续
		{ logfile.Write("stmtins 执行失败\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);  return false; }

		if (stmtsel.m_cda.rpc % 1000 == 0)
		{
			connloc.commit();
			PActive.UptATime();
		}
	}

	if (stmtsel.m_cda.rpc > 0)
	{
		logfile.Write("sync %s to %s (%d rows) in %.2fsec\n", starg.remotetname, starg.localtname, stmtsel.m_cda.rpc, Timer.Elapsed());
		// 处理最后未提交的数据
		connloc.commit();
		bcontinue = true;
	}

	return true;
}

// 从本地表starg.localtname中，获取自增字段的最大值，存放在全局变量maxkeyvalue中
bool _findlocalmaxkey()
{
	// 初始化待使用的变量的值
	maxkeyvalue = 0;

	sqlstatement stmt(&connloc);
	/* 以下方法没有用到sql语句中的max函数 */
	// 准备查询的sql语句
	// stmt.prepare("select %s from %s", starg.localkeycol, starg.localtname);

	// long tempkeyvalue;
	// // 绑定输出变量
	// stmt.bindin(1, &tempkeyvalue);

	// while (true)
	// {
	// 	// 输出查询的结果集
	// 	if (stmt.next() != 0;) break;

	// 	// 比较keycol的大小，将当前的最大值存放到maxkeyvalue中
	// 	if (tempkeyvalue > maxkeyvalue)
	// 		maxkeyvalue = tempkeyvalue;
	// }

	stmt.prepare("select max(%s) from %s", starg.localkeycol, starg.localtname);
	stmt.bindout(1, &maxkeyvalue);
	if (stmt.execute() != 0)
	{ logfile.Write("查询本地表的自增字段(%s)的最大值失败\n%s\n%s\n", starg.localkeycol, stmt.m_sql, stmt.m_cda.message); return false; }
	stmt.next();

	// test
	// logfile.Write("maxkeyvalue = %ld\n", maxkeyvalue);

	return true;
}

void _exit(int sig)
{
	logfile.Write("接收到信号%d，程序推出\n", sig);

	connloc.disconnect();
	connrem.disconnect();

	exit(sig);
}
