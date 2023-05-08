/*
 * 本程序是数据中心的公共功能模块，采用刷新的方法同步MySQL数据库之间的表。
 */
#include "_tools.h"

struct st_arg
{
	char localconnstr[101];     // 本地数据库的连接参数
	char charset[51];           // 数据库的字符集
	char fedtname[31];          // Federated表名
	char localtname[31];        // 本地表名
	char remotecols[1001];      // 远程表的字段列表
	char localcols[1001];       // 本地表的字段列表
	char where[1001];           // 同步数据的条件
	int  synctype;              // 同步方式：1-不分批同步；2-分批同步
	char remoteconnstr[101];    // 远程数据库的连接参数
	char remotetname[31];       // 远程表名
	char remotekeycol[31];      // 远程表的键值字段名
	char localkeycol[31];       // 本地表的键值字段名
	int  maxcount;              // 每批执行一次同步操作的记录数
	int  timeout;               // 本程序运行时的超时时间
	char pname[51];             // 本程序运行时的程序名
} starg;

CLogFile logfile;
CPActive PActive;
connection connloc;             // 本地数据库连接
connection connrem;             // 远程数据库连接

// 程序的帮助文档
void _help();
// 把xml解析到参数starg结构体中
bool _xmltoarg(char *strxmlbuffer);
// 业务处理主函数
bool _syncupdate();
// 程序推出函数
void _exit(int sig);

int main(int argc, char *argv[])
{
	if (argc != 3) { _help(); return -1; }

	// 关闭信号和IO
	// CloseIOAndSignal(true);
	signal(SIGINT, _exit); signal(SIGTERM, _exit);

	if (logfile.Open(argv[1],"a+") == false)
	{ printf("打开日志文件失败（%s）。\n", argv[1]); return -1; }

	// 把xml解析到参数starg结构中
	if (_xmltoarg(argv[2]) == false) return -1;

	// 当程序稳定了之后再启用，否则，调试程序时，可能会被守护进程误杀
	// PActive.AddPInfo(starg.timeout, starg.pname);

	if (connloc.connecttodb(starg.localconnstr, starg.charset) != 0)
	{ logfile.Write("connect database(%s) 失败\n%s\n", starg.localconnstr, connloc.m_cda.message); _exit(-1); }
	// logfile.Write("connect database(%s) 成功\n", starg.localconnstr);

	// 如果 remotecols 或者 localcols 为空， 那么我们需要使用 localtname表中的字段来填充它们
	if ( (strlen(starg.remotecols) == 0) || (strlen(starg.localcols) == 0) )
	{
		CTABCOLS TABCOLS;
		// 获取 localtname 表的所有字段名，存储在类的 m_allcols // 全部的字段名列表，以字符串存放，中间用半角的逗号分隔
		TABCOLS.allcols(&connloc, starg.localtname);
		if (strlen(starg.remotecols) == 0) strcpy(starg.remotecols, TABCOLS.m_allcols);
		if (strlen(starg.localcols)  == 0) strcpy(starg.localcols, TABCOLS.m_allcols);
	}

	// 业务处理主函数
	_syncupdate();

	return 0;
}

// 程序的帮助文档
void _help()
{
	printf("Use:syncupdate logfilename xmlbuffer\n\n");
	printf("Sample:\n");
	printf("/project/tools1/bin/procctl 10 /project/tools1/bin/syncupdate /log/idc/syncupdate_ZHOBTCODE2.log \"<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><charset>utf8</charset><fedtname>LK_ZHOBTCODE1</fedtname><localtname>T_ZHOBTCODE2</localtname><remotecols>obtid,cityname,provname,lat,lon,height/10,upttime,keyid</remotecols><localcols>stid,cityname,provname,lat,lon,altitude,upttime,id</localcols><synctype>1</synctype><timeout>50</timeout><pname>syncupdate_ZHOBTCODE2</pname>\"\n\n");

	// 因为测试的需要，xmltodb程序每次会删除LK_ZHOBTCODE1中的数据，全部的记录重新入库，id会变。
	// 所以以下脚本不能用id，要用obtid，用id会出问题，可以试试。
	printf("/project/tools1/bin/procctl 10 /project/tools1/bin/syncupdate /log/idc/syncupdate_ZHOBTCODE3.log \"<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><charset>utf8</charset><fedtname>LK_ZHOBTCODE1</fedtname><localtname>T_ZHOBTCODE3</localtname><remotecols>obtid,cityname,provname,lat,lon,height/10,upttime,keyid</remotecols><localcols>stid,cityname,provname,lat,lon,altitude,upttime,id</localcols><where>where obtid like '54%%%%'</where><synctype>2</synctype><remoteconnstr>10.0.8.4,root,DYT.9525ing,TestDB,3306</remoteconnstr><remotename>T_ZHOBTCODE1</remotename><remotekeycol>obtid</remotekeycol><localkeycol>stid</localkeycol><maxcount<10</maxcount><timeout>50</timeout><pname>syncupdate_ZHOBTCODE3</pname>\"\n\n");

	printf("/project/tools1/bin/procctl 10 /project/tools1/bin/syncupdate /log/idc/syncupdate_ZHOBTMIND2.log \"<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><charset>utf8</charset><fedtname>LK_ZHOBTMIND1</fedtname><localtname>T_ZHOBTMIND2</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><where>where ddatetime>timestampadd(minute,-120,now())</where><synctype>2</synctype><remoteconnstr>10.0.8.4,root,DYT.9525ing,TestDB,3306</remoteconnstr><remotetname>T_ZHOBTMIND1</remotetname><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><maxcount>300</maxcount><timeout>50</timeout><pname>syncupdate_ZHOBTMIND2</pname>\"\n\n");

	printf("本程序是数据中心的公共功能模块，采用刷新的方法同步mysql数据库之间的表。\n");

	printf("logfilename     本程序运行的日志文件。\n");
	printf("xmlbuffer       本程序运行的参数，用xml表示，具体如下：\n\n");


	printf("localconnstr    本地数据库的连接参数，格式：ip,username,password,dbname,port。\n");
	printf("charset         数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。\n");

	printf("fedtname        Federated表名。\n");
	printf("localtname      本地表名。\n");

	printf("remotecols      远程表的字段列表，用于填充在select和from之间，所以，remotecols可以是真实的字段，也可以是函数的返回值或者运算结果。\n"\
		   "                如果本参数为空，就用localtname表的字段列表填充。\n");
	printf("localcols       本地表的字段列表，与remotecols不同，它必须是真实存在的字段。如果本参数为空，就用localtname表的字段列表填充。\n");

	printf("where           同步数据的条件，本参数可以为空，如果为空，表示同步全部的记录。填充在delete本地表和select Federated表之后。\n"
		   "		注意：1）where中的字段必须同时在本地表和Federated表中；2）不要用系统时间作为条件\n");

	printf("synctype        同步方式：1-不分批同步；2-分批同步。\n");
	printf("remoteconnstr   远程数据库的连接参数，格式与localconnstr相同，当synctype==2时有效。\n");
	printf("remotetname     远程表名，当synctype==2时有效。\n");
	printf("remotekeycol    远程表的键值字段名，必须是唯一的，当synctype==2时有效。\n");
	printf("localkeycol     本地表的键值字段名，必须是唯一的，当synctype==2时有效。\n");

	printf("maxcount        每次执行一次同步操作的记录数，不能超过MAXPARAMS宏（在_mysql.h中定义，默认值为256），当synctype==2时有效。\n");

	printf("timeout         本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。\n");
	printf("pname           本程序运行时的进程名，尽可能采用易懂的、与其他进程不同的名称，方便排查故障。\n\n");
	printf("注意：\n1）remotekrycol和localkeycol字段的选取很重要，如果用了mysql的自增字段，那么在远程表中数据生成后自增字段的值不可改表，否则同步会失败；\n2）当远程表中存在delete操作时，无法分批同步，因为远程表的记录被delete后就找不到了，无法从本地表中执行delete操作。\n\n\n");
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

	// Federated表名。
	GetXMLBuffer(strxmlbuffer, "fedtname", starg.fedtname, 30);
	if (strlen(starg.fedtname) == 0)
	{ logfile.Write("fedtname 为空。\n"); return false; }

	// 本地表名。
	GetXMLBuffer(strxmlbuffer, "localtname", starg.localtname, 30);
	if (strlen(starg.localtname) == 0)
	{ logfile.Write("localtname 为空。\n"); return false; }

	// 远程表的字段列表，用于填充在select和from之间，所以，remotecols可以是真实的字段，也可以是函数的返回值或者运算结果。如果本参数为空，就用localtname表的字段列表填充。
	GetXMLBuffer(strxmlbuffer, "remotecols", starg.remotecols, 1000);

	// 本地表的字段列表，与remotecols不同，它必须是真实存在的字段。如果本参数为空，就用localtname表的字段列表填充。
	GetXMLBuffer(strxmlbuffer, "localcols", starg.localcols, 1000);

	// 同步数据的条件，即select语句的where部分。本参数可以为空，如果为空，表示同步全部的记录。
	GetXMLBuffer(strxmlbuffer, "where", starg.where, 1000);

	// 同步方式：1-不分批同步；2-分批同步。
	GetXMLBuffer(strxmlbuffer, "synctype", &starg.synctype);
	if (starg.synctype != 1 && starg.synctype != 2)
	{ logfile.Write("synctype 不正确。\n"); return false; }

	if (starg.synctype == 2)
	{
		// 远程数据库的连接参数，格式与localconnstr相同，当synctype==2时有效。
		GetXMLBuffer(strxmlbuffer, "remoteconnstr", starg.remoteconnstr, 100);
		if (strlen(starg.remoteconnstr) == 0)
		{ logfile.Write("remoteconnstr 为空。\n"); return false; }

		// 远程表名，当synctype==2时有效。
		GetXMLBuffer(strxmlbuffer, "remotetname", starg.remotetname, 30);
		if (strlen(starg.remotetname) == 0)
		{ logfile.Write("remotetname 为空。\n"); return false; }

		// 远程表的键值字段名，必须是唯一的，当synctype==2时有效。
		GetXMLBuffer(strxmlbuffer, "remotekeycol", starg.remotekeycol, 30);
		if (strlen(starg.remotekeycol) == 0)
		{ logfile.Write("remotekeycol 为空。\n"); return false; }

		// 本地表的键值字段名，必须是唯一的，当synctype==2时有效。
		GetXMLBuffer(strxmlbuffer, "localkeycol", starg.localkeycol, 30);
		if (strlen(starg.localkeycol) == 0)
		{ logfile.Write("localkeycol 为空。\n"); return false; }

		// 每次执行一次同步操作的记录数，不能超过MAXPARAMS宏（在_mysql.h中定义，默认值为256），当synctype==2时有效。
		GetXMLBuffer(strxmlbuffer, "maxcount", &starg.maxcount);
		if (starg.maxcount == 0)
		{ logfile.Write("maxcount 为空。\n"); return false; }
		if (starg.maxcount > MAXPARAMS)
		{ logfile.Write("maxcount 大于MAXPARAMS，已改为MAXPARAMS。\n"); starg.maxcount = MAXPARAMS; }
	}

	// 本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。
	GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
	if (starg.timeout == 0)
	{ logfile.Write("timeout 为空。\n"); return false; }

	// 本程序运行时的进程名，尽可能采用易懂的、与其他进程不同的名称，方便排查故障。
	GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
	if (strlen(starg.pname) == 0)
	{ logfile.Write("pname为空。\n"); return false; }

	return true;
}

// 业务处理主函数
bool _syncupdate()
{
	CTimer Timer;
	sqlstatement stmtdel(&connloc);     // 执行删除本地表中记录的SQL语句
	sqlstatement stmtins(&connloc);     // 执行向本地表插入数据的SQL语句

	// 如果是不分批同步，表示需要同步的数据量比较小，执行一次SQL语句就能完成
	if (starg.synctype == 1)
	{
		logfile.Write("sync %s to %s ... ", starg.fedtname, starg.localtname);
		// 先删除localtname表中满足where条件的记录
		stmtdel.prepare("delete from %s %s", starg.localtname, starg.where);
		if (stmtdel.execute() != 0)
		{ logfile.WriteEx("stmtdel 执行失败\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message); return false; }

		// 再把fedtname表中满足where条件的记录插入到localtname表中
		stmtins.prepare("insert into %s(%s) select %s from %s %s", starg.localtname, starg.localcols, starg.remotecols, starg.fedtname, starg.where);
		// 如果失败，一定不要忘记回滚事务
		if (stmtins.execute() != 0)
		{ logfile.WriteEx("stmtins 执行失败\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message); connloc.rollback(); return false; }

		logfile.WriteEx("%d rows in %.2fsec成功\n", stmtins.m_cda.rpc, Timer.Elapsed());
		connloc.commit();

		return true;
	}

	return true;
}

void _exit(int sig)
{
	logfile.Write("接收到信号%d，程序推出\n", sig);

	connloc.disconnect();
	connrem.disconnect();

	exit(sig);
}
