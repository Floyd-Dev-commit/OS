// demo.c
#include "unistd.h"
#include "sys/syscall.h"
#include "stdio.h"
#include <stdlib.h>
#define _SYSCALL_MYSETNICE_ 333
#define EFALUT 14

int main()
{
    int pid, flag, nicevalue;
    int prev_prio, prev_nice, cur_prio, cur_nice;
    int result;

    system("ps -l");
    printf("Please input variable(pid, flag, nicevalue): ");
    scanf("%d%d%d", &pid, &flag, &nicevalue);//读取进程ID,flag(读取还是修改nice值),nicevalue(新的value值)
    
    result = syscall(_SYSCALL_MYSETNICE_, pid, 0, nicevalue, &prev_prio,
                     &prev_nice);//调用mysetnice，prev_prio，prev_nice带回当前进程的prio,nice值.返回结果，如果为EFAULT报错，并return 1;
    if (result == EFALUT)
    {
        printf("ERROR!");
        return 1;
    }
    //开始调用
    if (flag == 1)//如果flag=1,修改nice值
    {
        syscall(_SYSCALL_MYSETNICE_, pid, 1, nicevalue, &cur_prio, &cur_nice);
        printf("Original priority is: [%d], original nice is [%d]\n", prev_prio,
               prev_nice);
        printf("Current priority is : [%d], current nice is [%d]\n", cur_prio,
               cur_nice);//打印修改前后的prio和nice值
    }
    else if (flag == 0)//如果flag=0，只打印先前的值，不调用setnice修改。
    {
        printf("Current priority is : [%d], current nice is [%d]\n", prev_prio,
               prev_nice);
    }
    else
{
	printf("Wrong Flag Value!!!\n");
}
    return 0;
}
