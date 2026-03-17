#include <semaphore.h>
#include <stdio.h>
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

int main()
{
	int msgid;
	key_t key = ftok("/home/jim",'a');
	msgid = msgget(key,O_RDONLY);
	msgctl(msgid,IPC_RMID,NULL);

	key = ftok("/home/jim",'b');
	msgid = msgget(key,O_RDONLY);
	msgctl(msgid,IPC_RMID,NULL);
	
	sem_unlink("QUEUE_A");
	sem_unlink("QUEUE_B");
	sem_unlink("RCV1");
	sem_unlink("RCV2");

	return 0;

}
