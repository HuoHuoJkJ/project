#include "_public.h"
#include "_mysql.h"

int main(int argc, char *argv[])
{
    connection connrem;
   	// 连接远程数据库
	connrem.connecttodb("127.0.0.1,root,DYT.9525ing,TestDB,3306", "utf8");

	char remkeyvalue[51];
	sqlstatement stmtsel(&connrem);
	// 从远程数据库中查找需要同步记录的key字段的值
	stmtsel.prepare("select obtid from T_ZHOBTCODE1 where obtid like '54%%'");
	stmtsel.bindout(1, remkeyvalue, 50);

	if (stmtsel.execute() != 0)
	{ logfile.Write("stmtsel 执行失败\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message); return false; }

    connection conn;
    conn.connecttodb("127.0.0.1,root,DYT.9525ing,TestDB1,3306", "utf8");
    sqlstatement stmt(&conn);
    char strvalue[10][51];
    stmt.prepare("insert into Test(stid,cityname,provname,lat,lon,altitude,upttime,id) select obtid,cityname,provname,lat,lon,height/10,upttime,keyid from LK_ZHOBTCODE1 where obtid in (54826,54836,54842,54843,54857,0,0,0,0,0)");

    for (int ii = 0; ii < 10; ii ++)
        stmt.bindin(ii+1, strvalue[ii], 50);

    int count = 0;
	memset(lockeyvalues, 0, sizeof(lockeyvalues));

	while (true)
	{
		// 获取需要同步数据的结果集
		if (stmtsel.next() != 0) break;

		strcpy(lockeyvalues[count], remkeyvalue);
		count++;

		// 每starg.maxcount条记录执行一次同步
		if (count == 10)
		{
			// 向本地表中插入记录
			if (stmtins.execute() != 0)
			// 执行从本地表中插入记录，一般不会报错
			// 如果报错，肯定是数据库的问题或同步参数配置不正确（如：表名写错等），流程不必继续
			{ logfile.Write("stmtins 执行失败\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);  return false; }

			logfile.Write("sync %s to %s (%d rows) in %.2fsec\n", starg.fedtname, starg.localtname, count, Timer.Elapsed());
			connloc.commit();
			count = 0;
			memset(lockeyvalues, 0, sizeof(lockeyvalues));
		}
	}

	// 如果count > 0，表示有剩余，同步剩余的部分
	if (count > 0)
	{
		// 向本地表中插入记录
		if (stmtins.execute() != 0)
		{ logfile.Write("stmtins 执行失败\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);  return false; }

		logfile.Write("sync %s to %s (%d rows) in %.2fsec\n", starg.fedtname, starg.localtname, count, Timer.Elapsed());
		connloc.commit();
		count = 0;
		memset(lockeyvalues, 0, sizeof(lockeyvalues));
	}

    return 0;
}