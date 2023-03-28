/* 
 * createtable.cpp  此程序用于演示开发框架操作MySQL数据库(创建表)
 */
#include "_mysql.h"

int main(void)
{
    connection conn;

    // 登录数据库，返回值：0-成功；其他值都是失败，存放了mysql的错误代码
    if (conn.connecttodb("127.0.0.1,root,DYT.9525ing!,TestDB,3306", "utf8") != 0)
    {
        printf("conn.connecttodb(%s) failed!\n", "127.0.0.1,root,DYT.9525ing!,TestDB,3306"); return -1;
    }
    
    // 操作SQL语句的对象
    sqlstatement stmt(&conn);
    
    // 准备创建表的sql语句
    /* 
     * int prepare(const char *fmt, ...);   是可变参数，可以多行书写SQL语句
     * SQL语句最后的分号可有可无，为了兼容性考虑，可以默认不加分号
     * SQL语句中不能有中文说明文字
     * 可以不用在prepare函数中判断sql语句是否正确，也就是不用关心prepare的返回值，我们会在执行(execute)函数中进行判断
     */
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