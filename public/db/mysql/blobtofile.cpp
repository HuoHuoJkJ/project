/* 
 * blobtofile.cpp       本程序演示将mysql表中的二进制大对象数据取出来存为文件的操作
 */
#include "_mysql.h"

int main(void)
{
    connection conn;
    conn.connecttodb("127.0.0.1,root,DYT.9525ing!,TestDB,3306", "utf8");
    sqlstatement stmt(&conn);

    struct st_girls
    {
        long id;
        char pic[100000];
        unsigned long picsize;
    } stgirls;

    stmt.prepare(
                "select id,pic from girls where id in (1,2)"
                );
    stmt.bindout(1, &stgirls.id);
    stmt.bindoutlob(2, stgirls.pic, sizeof(stgirls.pic), &stgirls.picsize);

    if (stmt.execute() != 0)
    { printf("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return -1; }
    
    while (true)
    {
        memset(&stgirls, 0, sizeof(struct st_girls));
        
        if (stmt.next() != 0) break;
        
        char filename[301]; memset(filename, 0, sizeof(filename));
        sprintf(filename, "%d_out.jpg", stgirls.id);
        
        buftofile(filename, stgirls.pic, stgirls.picsize);
    }
    
    printf("select 数据成功，本次查询了girls表的%ld条数据\n", stmt.m_cda.rpc);

    return 0;
}