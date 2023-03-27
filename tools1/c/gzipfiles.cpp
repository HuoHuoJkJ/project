#include "_public.h"

void EXIT(int sig);

CDir Dir;

int main(int argc, char *argv[])
{
    // 程序帮助文档
    if (argc != 4)
    {
        printf("\n");
        printf("Using:gzipfiles pathname matchstr timeout\n\n");

        printf("Example:/project/tools1/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
        printf("        /project/tools1/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*json\" 0.01\n");
        printf("        /project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
        printf("        /project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*json\" 0.01\n\n");

        printf("这是一个工具程序，用于压缩历史的数据文件或日志文件。\n");
        printf("本程序把pathname目录及子目录中timeout天之前的匹配matchstr文件全部压缩，timeout可以是小数。\n");
        printf("本程序不写日志文件，也不会在控制台输出任何信息。\n");
        printf("本程序调用/usr/bin/gzip命令压缩文件。\n\n\n");

        return -1;
    }
    // 关闭信号和输入输出
    // CloseIOAndSignal(true);   // 为了方便调试程序，暂时不启用 
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    // 确定传入的文件超时时间
    char strTimeOut[21];
    LocalTime(strTimeOut, "yyyy-mm-dd hh24:mi:ss", 0 - (int)(atof(argv[3])*24*60*60));
    
    // 打开目录
    if (Dir.OpenDir(argv[1], argv[2], 10000, true) == false)
    {
        printf("Dir.OpenDir(%s) failed!\n", argv[1]);
        return -1;
    }
    
    char strCmd[1024];

    while (true)
    {
        // 得到一个文件的信息
        if (Dir.ReadDir() == false) break;
        
        // 输出文件信息
        // printf("DirName = %s,\tFileName = %s,\tFullFileName = %s,\tFileSize = %d,\tModifyTime = %s,\tCreateTime = %s,\tAccessTime = %s\n", Dir.m_DirName, Dir.m_FileName, Dir.m_FullFileName, Dir.m_FileSize, Dir.m_ModifyTime, Dir.m_CreateTime, Dir.m_AccessTime);
        // printf("FullFileName = %s\n", Dir.m_FullFileName);
        if ( (strcmp(Dir.m_ModifyTime, strTimeOut) < 0) && (MatchStr(Dir.m_FileName, "*.gz") == false) )
        {
            SNPRINTF(strCmd, sizeof(strCmd), 1000, "/usr/bin/gzip -f %s 1>/dev/null 2>/dev/null", Dir.m_FullFileName);
            if (system(strCmd) == 0)
                printf("%s 压缩成功。\n", Dir.m_FullFileName);
            else
                printf("%s 压缩失败！\n", Dir.m_FullFileName);
        }
    }


}

void EXIT(int sig)
{
    printf("接收到%d信号，即将退出程序。\n", sig);
    exit(0);
}
