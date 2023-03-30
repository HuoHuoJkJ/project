/* 
 * inserttablee.cpp     此程序用于演示插入表内容的操作
 */
#include "_mysql.h"

int main(void)
{
    connection conn;
    if (conn.connecttodb("127.0.0.1,root,DYT.9525ing!,TestDB,3306", "utf8") != 0)
    { printf("conn.connecttodb(127.0.0.1,root,DYT.9525ing!,TestDB,3306) failed.\n"); return -1; }

    // id     | bigint(10)  
    // name   | varchar(10) 
    // weight | decimal(8,2)
    // btime  | datetime        
    struct st_girls
    {
        long   id;          // 编号
        char   name[31];    // 姓名
        double weight;      // 体重
        char   btime[20];   // 报名时间
    } stgirls;

    sqlstatement stmt(&conn);
    
    stmt.prepare(
                "delete from girls" 
                );

    if (stmt.execute() != 0)
    { printf("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return -1; }
    printf("delete table girls successed\n%d rows been deleted\n", stmt.m_cda.rpc);

    stmt.prepare(
                 "insert into girls(id,name,weight,btime) values(:1,:2,:3,str_to_date(:4,'%%Y-%%m-%%d %%H:%%i:%%s'))"
                //  "insert into girls(id,name,weight,btime) values(?,?,?,str_to_date(?,'%%Y-%%m-%%d %%H:%%i:%%s'))"
                );
    /* 
     * 参数的序号从1开始，连续、递增，参数也可以用问号表示，但是，问号的兼容性不好，不建议
     * SQL语句中的右值才能作为参数，表名、字段名、关键字、函数名等都不能作为参数
     */
    // printf("=%s=", stmt.m_sql);
    // 绑定输入变量
    stmt.bindin(1, &stgirls.id);
    stmt.bindin(2,  stgirls.name, 30);
    stmt.bindin(3, &stgirls.weight);
    stmt.bindin(4,  stgirls.btime, 19);
    
    int ii;
    for (ii = 0; ii < 5; ii++)
    {
        memset(&stgirls, 0, sizeof(struct st_girls));
        stgirls.id = ii + 1;
        sprintf(stgirls.name, "西施%05dgirl", ii + 1);
        stgirls.weight = ii + 44.25;
        snprintf(stgirls.btime, sizeof(stgirls.btime), "2023-3-28 12:23:%02d", ii);

        if (stmt.execute() != 0)
        {
            printf("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return -1;
        }
        printf("insert %02d row message successed!\n", stmt.m_cda.rpc);
    }
    printf("insert successed\n");
    
    conn.commit();

    return 0;
}