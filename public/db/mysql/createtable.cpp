/* 
 * createtable.cpp  此程序用于演示开发框架操作MySQL数据库(创建表)
 */
#include "_mysql.h"

int main(void)
{
    connection conn;
    if (conn.connecttodb("127.0.0.1,root,DYT.9525ing!,mysql,3306", "utf8") != 0)
    {
        printf("conn.connecttodb(%s) failed!\n", "127.0.0.1,root,DYT.9525ing!,mysql,3306"); return -1;
    }
    
    sqlstatement stmt(&conn);
    
    stmt.prepare("create table girls(id        bigint(10),\
                                     name      varchar(10),\
                                     weight    decimal(8,2),\
                                     btime     datetime,\
                                     memo      longtext,\
                                     pic       longblob,\
                                     primary   key (id))");
    
    // 执行SQL语句，一定要判断返回值，0-成功，其他-失败
    // 失败代码在stmt.m_cda.rc中，失败描述在stmt.m_cda.message中
    if (stmt.execute() != 0)
    {
        printf("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return -1;
    }

    return 0;
}