/* 
 * 测试printf函数分行书写
 */
#include <stdio.h>

int main(void)
{
    printf("哈哈就看见"
    "哈哈就看见、"
    "活活就看见\n");

    printf("哈哈就看见\
哈哈就看见、\
                哈哈就看见、\
    活活就看见");

    return 0;
}