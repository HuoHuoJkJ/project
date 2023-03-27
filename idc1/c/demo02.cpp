/* 
 * sizeof获取argv[1]参数的大小
 * 只输出8，因为他是形参
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("请输入一个参数\n");
        return -1;
    }
    printf("sizeof(argc[1]) = %d\n", sizeof(argv[1]));
    printf("strlen(argv[1]) = %d\n", strlen(argv[1]));
    /* 
     * 这样会出错，因为我试图使用 strcpy 函数将字符串 “haha” 复制到未初始化的指针 arr[1] 所指向的内存中。在使用 strcpy 函数之前，我需要为 arr[1] 分配足够的内存来存储字符串 “haha”。
     */
    // char *arr[2];
    // strcpy(arr[1], "haha");
    // printf("sizeof(arr[1]) = %d\n", sizeof(arr[1]));

    char *arr[2];
    arr[1] = (char *)malloc(strlen("haha") + 1);
    strcpy(arr[1], "haha");
    printf("sizeof(arr[1]) = %d\n", sizeof(arr[1]));
    printf("strlen(arr[1]) = %d\n", strlen(arr[1]));
    free(arr[1]);

    return 0;
}