/* 
 * updatetable.cpp      此程序用于演示更新数据库表中内容
 */
#include "_mysql.h"

int main(void)
{
    connection conn;
    conn.connecttodb("127.0.0.1,root,DYT.9525ing!,TestDB,3306", "utf8");
    
    struct st_girls
    {
        long    id;
        char    name[31];
        double  weight;
        char    btime[20];
    } stgirls;
    
    sqlstatement stmt(&conn);
    stmt.prepare(
                "update girls set name=:1,weight=:2,btime=str_to_date(:3,'%%Y-%%m-%%d %%H:%%i:%%s') where id=:4"
                );
    stmt.bindin(1,  stgirls.name, 30);
    stmt.bindin(2, &stgirls.weight);
    stmt.bindin(3,  stgirls.btime, 19);
    stmt.bindin(4, &stgirls.id);

    for (int ii = 0; ii < 5; ii++)
    {
        memset(&stgirls, 0, sizeof(struct st_girls));
        
        stgirls.id = ii + 1;
        sprintf(stgirls.name, "貂蝉%05dgirl", ii + 1);
        stgirls.weight = ii + 52.33;
        sprintf(stgirls.btime, "2021-3-28 12:33:%02d", ii);
        
        if (stmt.execute() != 0)
        { printf("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return -1; }
        printf("Update %d line successed.\n", stmt.m_cda.rpc);
    }
    printf("Update table girls successed.\n");
    
    conn.commit();

    return 0;
}