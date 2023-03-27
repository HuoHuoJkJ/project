#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX 10

int main(void)
{
    printf("MAX = %d", MAX);
    (int)MAX = 20;
    printf("MAX = %d", MAX);


    return 0;
}