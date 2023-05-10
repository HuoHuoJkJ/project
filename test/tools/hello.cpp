#include "_public.h"

int main(int argc, char *argv[])
{
    CLogFile logfile;
    CPActive PActive;

    PActive.AddPInfo(10, "hello");

    logfile.Open(argv[1], "a+");
    logfile.Write("hello world");

    while (true)
    {
        PActive.UptATime();
    }

    return 0;
}