#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

//module加载时运行__init
static int __init show_all_kernel_thread_init(void)
{
    struct task_struct *p;
    
    //让输出好看点～
    printk("%-20s%-6s%-6s%-6s%-6s", "Name", "PID", "State", "Prio", "PPID");
    printk("--------------------------------------------");
    
    for_each_process(p)//一个宏定义，遍历进程链表中的所有进程
    {
	/*
	内存描述符（memory descriptor) mm_struct，抽象并描述了Linux视角下管理进程地址空间的所有信息
	*/
	/*
		用户进程和内核线程（kernel thread)都是task_struct的实例，唯一的区别是kernel thread是没有进程地址空间的，内核线程也没有mm描述符的，所以内核线程的tsk->mm域是空（NULL）
	*/
        if (p->mm == NULL)//如果是内核线程则输出相关信息
        {
            printk("%-20s%-6d%-6d%-6d%-6d", p->comm, p->pid, p->state, p->prio,
                   p->parent->pid);
        }
    }
    return 0;
}

//module模块退出时运行__exit
static void __exit show_all_kernel_thread_exit(void)
{
    printk("[ShowAllKernelThread] Module Uninstalled.");
}

module_init(show_all_kernel_thread_init);
module_exit(show_all_kernel_thread_exit);
