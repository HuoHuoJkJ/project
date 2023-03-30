/* 
 * obtcodetodb.cpp      本程序用于把全国站点参数数据保存到数据库T_ZHOBTCODE中
 */
#include "_public.h"
#include "_mysql.h"

int main(int argc, char *argv[])
{
    // 程序的帮助文档
    if (argc != 3)
    {
        printf("Use:obtcodetodb logfile ip username password port db character");
        return -1;
    }

    // 处理程序运行时的退出信号和IO

    // 打开日志文件
    
    // 把全国站点参数文件加载到容器vstcode中
    
    // 连接数据库 

    // 准备插入表的SQL语句

    // 准备更新表的SQL语句
    
    // 遍历容器vstcode
    for (int ii = 0; ii < vstcode.size(); ii++)
    {
        // 从容器中取出一条记录到结构体stcode中
        
        // 执行插入的SQL语句
        
        // 如果记录已存在，执行更新SQL语句
    }
    
    // 对表的修改进行总结。总修改、插入、更新
    
    // 执行提交操作

    return 0;
}