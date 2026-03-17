#include "mem_admin.h"
#include <semaphore.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>

int main()
{
	int shmid = shmget(1024,sizeof(mem_area),0666|IPC_CREAT);
	mem_area* MA=shmat(shmid,NULL,0);
	shmctl(shmid,IPC_RMID,(void*)MA);
	sem_unlink("PATH1");
	sem_unlink("PATH2");
	sem_unlink("PATH3");
	sem_unlink("PATH4");
	sem_unlink("EMPTY");
	sem_unlink("FULL");
	sem_unlink("OCCUPY");	
	return 0;
}
