#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>

sem_t* queue_a;
sem_t* queue_b;
sem_t* rcv1;

typedef struct{
    long type;
	char origin[10];
    char text[100];
}Msg;

int main()
{
	system("clear");
	puts("I am snd1");
	queue_a = sem_open("QUEUE_A",O_CREAT,0666,1);
	queue_b = sem_open("QUEUE_B",O_CREAT,0666,1);
	rcv1 = sem_open("RCV1",O_CREAT,0666,0);
	/*snd1_note_rcv1 = sem_open("SND1_NOTE_RCV1",O_CREAT,0666,0);
	snd1_note_rcv2 = sem_open("SND1_NOTE_RCV2",O_CREAT,0666,0);*/

    int flag=1;
    do
    {
    	if(flag==0) 
		{
			sem_close(queue_a);
			sem_close(queue_b);
			sem_close(rcv1);
			break;
		}
		int Dest=0;
		printf("请输入发送对象,rcv1=1,rcv2=2.");
		scanf("%d",&Dest);
		if(Dest==1)
		{
			sem_wait(queue_a);
    		key_t key = ftok("/home/jim",'a');	
    		int msgid = msgget(key,IPC_CREAT|O_WRONLY|0777);
    		if(msgid<0)
    		{   
        		perror("msgget error!");
        		exit(-1);
    		}   	
    		Msg m;
			strcpy(m.origin,"snd1");
    		puts("请输入你要发送的消息类型、内容！");
    		scanf("%ld%s",&m.type,m.text);
    		msgsnd(msgid,&m,sizeof(m)-sizeof(m.type),0);
			sem_post(queue_a);
			sem_post(rcv1);
		}	
		else if(Dest==2)
		{
			sem_wait(queue_b);
			key_t key = ftok("/home/jim",'b');
	
    		int msgid = msgget(key,IPC_CREAT|O_WRONLY|0777);
    		if(msgid<0)
    		{   
        		perror("msgget error!");
        		exit(-1);
    		}   
	
    		Msg m;
			strcpy(m.origin,"snd1");
    		puts("请输入你要发送的消息类型、内容！");
    		scanf("%ld%s",&m.type,m.text);
    		msgsnd(msgid,&m,sizeof(m)-sizeof(m.type),0);
			sem_post(queue_b);
			sem_post(rcv1);	
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
