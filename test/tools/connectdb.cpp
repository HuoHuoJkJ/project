#include "_public.h"
#include "_ooci.h"

int main(void)
{
    CTimer Timer;
    connection conn;

    for (int ii = 0; ii < 200000; ii ++)
    {
        if (conn.connecttodb("scott/dyting9525@snorcl11g_43", "Simplified Chinese_China.AL32UTF8") != 0)
            printf("创建数据库失败\n");
        printf("connect ok\n");
    }
    printf("Elasped()=%.2f\n", Timer.Elapsed());
    sleep(10);

    return 0;
}