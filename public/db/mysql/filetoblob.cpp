/* 
 * filetoblob.cpp       本程序用于演示插入二进制大对象(照片等)
 */
#include "_mysql.h"

int main(void)
{
    connection conn;
    conn.connecttodb("127.0.0.1,root,DYT.9525ing!,TestDB,3306", "utf8");
    
    struct st_girls
    {
        long id;                // 编号
        char pic[100000];       // 图片内容
        unsigned long picsize;  // 图片占用字节数
    } stgirls;

    sqlstatement stmt(&conn);
    stmt.prepare(
                "update girls set pic=:1 where id=:2"
                );
    stmt.bindinlob(1, stgirls.pic, &stgirls.picsize);
    stmt.bindin(2, &stgirls.id);
    
    for (int ii = 1; ii < 3; ii++)
    {
        memset(&stgirls, 0, sizeof(stgirls));
        
        stgirls.id = ii;
        if (ii == 1) stgirls.picsize = filetobuf("1.jpg", stgirls.pic);
        if (ii == 2) stgirls.picsize = filetobuf("2.jpg", stgirls.pic);
        
        if (stmt.execute() != 0)
        { printf("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); return -1; }
        printf("在id=%d的位置插入pic成功，%d行被修改\n", stgirls.id, stmt.m_cda.rpc);
    }
    
    printf("update girls表成功\n");
    conn.commit();

    return 0;
}