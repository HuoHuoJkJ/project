/* 
 * 测试utime的使用，为什么调用开发框架中的UTime函数，文件的时间戳变成了1970
 */
#include <stdio.h>
#include <sys/types.h>
#include <utime.h>
#include <time.h>

bool UTime(const char *filename)
{
    struct tm tm;
    strptime("20210710123000", "%Y%m%d%H%M%S", &tm);
    time_t mtime = mktime(&tm);

    struct utimbuf stutimbuf;
    stutimbuf.actime = stutimbuf.modtime = mtime;

    if (utime(filename, &stutimbuf) != 0) return false;

    return true;
}

int main(void)
{
    UTime("/tmp/idc1/surfdata/SURF_ZH_2021071012300_20848.xml");
    return 0;
}