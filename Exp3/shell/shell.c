#include<unistd.h>
#include<stdio.h>

char *sys_cmd [4] = {"exit", "cmd1", "cmd2", "cmd3"};  
  
/*获取自定义shell接收的用户命令编号*/
int get_cmd_id(char *cmd){  
    int i;  
    for (i = 0; i < 4; i++) {  
        if (strcmp(cmd, sys_cmd[i]) == 0) {   
            return i;  
        }  
    }  
    return -1;  
}  
  
void z_fork(int cmd_id){
/*fork()函数用于克隆进程，并返回值， 
1）在父进程中，fork返回新创建子进程的进程ID；
    2）在子进程中，fork返回0；
3）克隆失败返回负值*/  
    pid_t pid = fork();//父进程在z_fork()函数中只进行了克隆操作  
    if (pid < 0) {  
        printf("[*] Error: fork() return error\n");//如果进程克隆失败返回错误信息&&exit(0);  
        exit(0);  
    }
	else if (pid == 0) 
    	{  
	/*此处使用fork()函数的原因如下：自定义shell是父进程，需要循环接受用户输入，执行的命令作为子进程运行，互不干扰*/
        // 子进程 
        switch (cmd_id) {  
        case 1:  
            /*execl,执行一个新的进程替代execl的主调进程。*/
            execl("./cmd1", "", NULL); //param1->执行的文件路径，param2->传入新进程的参数。       
            break;  
        case 2:  
            execl("./cmd2", "", NULL);  
            break;  
        case 3:  
            execl("./cmd3", "", NULL);  
            break;  
        default:  
        printf("[*] Error: command not found!!!\n");  
            exit(0);  //子进程执行完毕后必须exit(0),否则回到main函数中会继续while(1)循环，与父进程冲突。
    	}  
    }   
}  
  
int main(){  
    char cmd[50];  
    int cmd_id = -1;  
    while (1) {  
    printf("-----------------------------------\n");  
        printf("Input command: \n> ");  
        scanf("%s", cmd);  
        cmd_id = get_cmd_id(cmd);  
        if (cmd_id == -1) {  
            printf("[*] Error: command not found!!!\n");//输入非法指令报错  
        } else if (cmd_id == 0) {  //如果用户输入exit（指令编号0），提示安全退出
            printf("[*] Exit safely.\n");  
            exit(0);  
        } else {  
            z_fork(cmd_id);  
        }  
        wait(NULL);  
    }  
}  
