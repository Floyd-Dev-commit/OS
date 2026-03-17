#include "mem_admin.h"
#include <semaphore.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>

//semaphores as follow:
sem_t* path1;
sem_t* path2;
sem_t* empty;
sem_t* full;
sem_t* occupy;

int main()
{
	system("clear");
	puts("I am snd2");
	

	path1 = sem_open("PATH1",O_CREAT,0666,0);
	path2 = sem_open("PATH2",O_CREAT,0666,0);
	empty = sem_open("EMPTY",O_CREAT,0666,BLOCK_MAX_SIZE);
	full = sem_open("FULL",O_CREAT,0666,0);
	occupy = sem_open("OCCUPY",O_CREAT,0666,1);

    int flag=1;
    do
    {
    	if(flag==0) 
		{
			//semaphores close.
			sem_close(path1);
			sem_close(path2);
			sem_close(empty);
			sem_close(full);
			sem_close(occupy);
			break;
		}
		int Dest=0;
		printf("请输入发送对象,rcv1=1,rcv2=2.");
		scanf("%d",&Dest);
		if(Dest==1)
		{			
			//Get the func of mem_area;(MA);
			sem_wait(empty);
			sem_wait(occupy);
			int shmid = shmget(1024,sizeof(mem_area),0666|IPC_CREAT);
			mem_area* MA=shmat(shmid,NULL,0);
			puts("请输入你要发送的消息！");
			char message[MSG_MAX_SIZE];
			scanf("%s",message);
			char origin[10];
			strcpy(origin,"snd2");
			send_message(MA,1,message,origin);
			sem_post(occupy);
			sem_post(full);
			sem_post(path1);		
		}	
		else if(Dest==2)
		{
			sem_wait(empty);
			sem_wait(occupy);
			int shmid = shmget(1024,sizeof(mem_area),0666|IPC_CREAT);
			mem_area* MA=shmat(shmid,NULL,0);
			puts("请输入你要发送的消息！");
			char message[MSG_MAX_SIZE];
			scanf("%s",message);
			char origin[10];
			strcpy(origin,"snd2");
			send_message(MA,2,message,origin);
			sem_post(occupy);
			sem_post(full);
			sem_post(path2);
		}
		else
		{
			puts("您的输入有误，请重新输入发送对象！");
		}
	
    	printf("继续发送信息？输入1继续，输入0退出!");
    	scanf("%d",&flag);
    }while(flag==1);

    return 0;
}
