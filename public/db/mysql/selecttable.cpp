/* 
 * selecttable.cpp      此程序用于演示查询数据库中的内容
 */
#include "_mysql.h"

int main(void)
{
    connection conn;
    conn.connecttodb("127.0.0.1,root,DYT.9525ing!,TestDB,3306", "utf8");
    
    sqlstatement stmt(&conn);
    
    int iminid, imaxid;
    struct st_girls
    {
        long    id;
        char    name[31];
        double  weight;
        char    btime[20];
    } stgirls;

    stmt.prepare(
                // "select id,name,weight,date_format(btime,'%%Y-%%m-%%d %%H:%%i:%%s') from girls where id in (2,3,4)" 
                "select id,name,weight,date_format(btime,'%%Y-%%m-%%d %%H:%%i:%%s') from girls where id>=:1 and id<=:2"
                );
    /* 
     * 如果SQL语句的主体没有改变，只需要prepare()一次就可以了
     * 结果集中的字段，调用bindour()绑定变量的地址
     * bindout()方法的返回值固定为0，不用判断返回值
     * 如果SQL语句的主体已改变，prepare()后，需要重新用bindout()绑定变量
     * 调用execute()方法执行SQL语句，然后再循环调用next()方法获取结果集中的记录
     * 每调用一次next()方法，从结果集中获取一条记录，字段内容保存在已绑定的变量中
     */
    // 为SQL语句绑定输入变量地址
    stmt.bindin(1, &iminid);
    stmt.bindin(2, &imaxid);
    // 为SQL语句绑定输出变量地址
    stmt.bindout(1, &stgirls.id);
    stmt.bindout(2,  stgirls.name, 30);
    stmt.bindout(3, &stgirls.weight);
    stmt.bindout(4,  stgirls.btime, 19);
    
    iminid = 1;
    imaxid = 5;
    
    if (stmt.execute() != 0)
    { printf("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return -1; }
    
    while (true)
    {
        memset(&stgirls, 0, sizeof(struct st_girls));
        
        if (stmt.next() != 0) break;
        
        printf("id=%ld, name=%s, weight=%.02lf, btime=%s\n", stgirls.id, stgirls.name, stgirls.weight, stgirls.btime);
    }
    printf("select %d rows\n", stmt.m_cda.rpc);

    return 0;
}