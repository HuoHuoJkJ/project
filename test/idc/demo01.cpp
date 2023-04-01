/* 
 * 验证转译!
 */
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        // printf("Use:./demo01 \"hahajiukanjian,!,dyt\"\n");
        printf("Use:./demo01 \"hahajiukanjian,\!,dyt\"\n");
        return -1;
    }
    printf("%s\n", argv[1]);

    return 0;
}