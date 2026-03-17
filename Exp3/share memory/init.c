#include "mem_admin.h"
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
	init_mem_area(MA);
	return 0;
}
