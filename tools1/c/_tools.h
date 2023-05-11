#ifndef _TOOLS_H
#define _TOOLS_H
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
	int m_maxcollen;	// 全部列中最大的长度

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

#endif