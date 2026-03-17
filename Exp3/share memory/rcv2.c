#include "mem_admin.h"
#include <semaphore.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>

sem_t* path2;
sem_t* empty;
sem_t* full;
sem_t* occupy;

int main()
{
	system("clear");
	puts("I am rcv2");

	path2 = sem_open("PATH2",O_CREAT,0666,0);
	empty = sem_open("EMPTY",O_CREAT,0666,BLOCK_MAX_SIZE);
	full = sem_open("FULL",O_CREAT,0666,0);
	occupy = sem_open("OCCUPY",O_CREAT,0666,1);

    int flag=1;
	puts("输入1开始接收消息，输入0退出！");
	scanf("%d",&flag);
    do
    {
    	if(flag==0)
		{ 
			sem_close(path2);
			sem_close(empty);
			sem_close(full);
			sem_close(occupy);	
			break; 
		}
		else if(flag==1)
		{  
			sem_wait(path2);
			sem_wait(full);
			sem_wait(occupy);
			int shmid = shmget(1024,sizeof(mem_area),0666|IPC_CREAT);
			mem_area* MA=shmat(shmid,NULL,0);
			char message[MSG_MAX_SIZE];
			char origin[10];
			receive_message(MA,2,message,origin);
			sem_post(occupy);
			sem_post(empty);	
			printf("%s:%s\n",origin,message);
		}
		else
		{
			puts("输入有误，请重新输入消息来源！");
		}
		puts("继续接收信息？输入1继续，输入0退出!");
    	scanf("%d",&flag);
    }while(flag==1);

    return 0;
}
