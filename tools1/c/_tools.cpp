#include "_tools.h"

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
	m_maxcollen = 0;
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

		if (m_maxcollen < stcolumns.collen) m_maxcollen = stcolumns.collen;

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
	m_maxcollen = 0;
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