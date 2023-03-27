/* 
 * 测试0.04天的实际秒数
 */
#include "_public.h"

int main(void)
{
    int ii = (int)(0.04 * 24 * 60 * 60);
    printf("%d\n", ii);
    char strTime[30];
    LocalTime(strTime, "yyyy-mm-dd hh24:mi:ss", 0-ii);
    printf("%s\n", strTime);

    return 0;
}