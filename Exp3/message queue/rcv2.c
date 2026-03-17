#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <linux/errno.h>
#include <semaphore.h>

sem_t* queue_b;
sem_t* rcv2;

typedef struct{
    long type;
	char origin[10];
    char text[100];
}Msg;

int main()
{
	system("clear");
	puts("I am rcv2");
	
	queue_b=sem_open("QUEUE_B",O_CREAT,0666,1);
	rcv2=sem_open("RCV2",O_CREAT,0666,0);
    
	int flag=1;
	int msgid=0;

	puts("输入1开始接收消息，输入0退出!");	
	scanf("%d",&flag); 
    do
    {
    	if(flag==0)
		{ 
			sem_close(queue_b);
			sem_close(rcv2);		
			break; 
		}
		else if(flag==1)
		{  
			sem_wait(rcv2);
			sem_wait(queue_b);
			key_t key = ftok("/home/jim",'b');
			msgid = msgget(key,O_RDONLY);

    		if(msgid<0)
    		{   
				puts("消息队列不存在！");        
				exit(-1);
    		}   

    		Msg rcv;
    		msgrcv(msgid,&rcv,sizeof(rcv)-sizeof(rcv.type),0,0);
			printf("信息来源：%s，类型:%ld,内容:%s\n",rcv.origin,rcv.type,rcv.text);
			sem_post(queue_b);
		}
		else
		{
			printf("输入有误，请重新输入消息来源！");
		}
		printf("继续接收信息？输入1继续，输入0退出!");
    	scanf("%d",&flag);
    }while(flag==1);

    return 0;
}
