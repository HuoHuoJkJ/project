/* 
 * 程序名：crtsurfdata1.cpp  本程序用于生成全国气象站点观测的分钟数据
 * 作者：丁永涛
 */
#include "_public.h"

CLogFile logfile(10);

int main(int argc, char *argv[])
{
    // inifile outpath logfile 气象站点参数 气象站点观测数据参数 日志文件
    if (argc != 4)    
    {
        printf("Use:./crtsurfdata1 inifile outpath logfile\n");
        printf("Example:/project/idc1/bin/crtsurfdata1 /project/idc1/ini/stcode.ini /tmp/surfdata /log/idc/crtsurfdata1.log\n");
        
        printf("inifile 全国气象站点参数文件名。\n");
        printf("outpath 全国气象站点数据文件存放的目录。\n");
        printf("logfile 本程序运行的日志文件名。\n\n");
        
        return -1;
    }
    
    if (logfile.Open(argv[3], "a+", false) == false)
    {
        printf("logfile.Open(%s) failed.\n", argv[3]);
        return -1;
    }
    
    logfile.Write("crtsurfdata1 开始运行\n");

    // 在这里插入业务代码。
    // for (int i = 0; i<10000000; i++)
        // logfile.Write("这是第%010d条日制记录。\n", i);

    logfile.WriteEx("crtsurfdata1 运行结束\n");

    return 0;
}