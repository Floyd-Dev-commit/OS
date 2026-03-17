// show_task_family.c
// created by 19-03-26
// Arcana
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/pid.h>
#include <linux/list.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
static int pid;
module_param(pid, int, 0644);

static int __init show_task_family_init(void)//定义为static对特定文件以外的文件透明，__init表示只初始化一次，加载后丢弃函数，释放空间。
{
    struct pid *ppid;
    struct task_struct *p;
    struct task_struct *pos;
    char *ptype[4] = {"[I]", "[P]", "[S]", "[C]"};
    //标识不同的进程类型I-本进程/P-父进程
    
    //通过pid找到对应的pcb
    ppid = find_get_pid(pid);
    //输入检测，如果pid有误，就输出错误信息
    if (ppid == NULL)
    {
        printk("[ShowTaskFamily] Error, PID not exists.\n");
        return -1;
    }
    p = pid_task(ppid, PIDTYPE_PID);

    // 让输出好看点～
    printk("%-10s%-20s%-6s%-6s\n", "Type", "Name", "PID", "State");
    printk("------------------------------------------\n");

    // 本进程信息
    printk("%-10s%-20s%-6d%-6d\n", ptype[0], p->comm, p->pid, p->state);

    // 父进程信息
    printk("%-10s%-20s%-6d%-6d\n", ptype[1], p->real_parent->comm,
           p->real_parent->pid, p->real_parent->state);

    // Siblings
    // 遍历父进程的子进程，即本进程的兄弟进程，输出信息
    // pid一致（表示本进程）跳过。
    list_for_each_entry(pos, &(p->real_parent->children), sibling)
    {
        if (pos->pid == pid)
            continue;
        printk("%-10s%-20s%-6d%-6d\n", ptype[2], pos->comm, pos->pid,
               pos->state);
    }

    // Children
    // 遍历”我“的子进程，输出信息
    list_for_each_entry(pos, &(p->children), sibling)
    {
        printk("%-10s%-20s%-6d%-6d\n", ptype[3], pos->comm, pos->pid,
               pos->state);
    }

    return 0;
}

static void __exit show_task_family_exit(void)
{
    printk("[ShowTaskFamily] Module Uninstalled.\n");
}

module_init(show_task_family_init);
module_exit(show_task_family_exit);
