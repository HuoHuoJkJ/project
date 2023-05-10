#include "_public.h"

CPActive PActive;

int main(int argc, char *argv[])
{
    char *pargv[argc];
    for (int i = 1; i < argc; i++)
        pargv[i-1] = argv[i];
    pargv[argc-1] = NULL;

    execv(argv[1], pargv);

    // PActive.AddPInfo(10, "test.tools.checkpro");

    // while (true)
    // {
    //     sleep(5);
    //     PActive.UptATime();
    // }

    return 0;
}