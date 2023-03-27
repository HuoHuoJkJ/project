#include "_public.h"

void EXIT(int sig);

int main(int argc, char *argv[])
{
}

void EXIT(int sig)
{
    printf("接收到了%d信号，程序将终止\n", sig);
    exit(0);
}
