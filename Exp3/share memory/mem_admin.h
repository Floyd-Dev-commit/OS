#ifndef _MEM_ADMIN_
#define _MEM_ADMIN_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>   
#define BLOCK_MAX_SIZE 512
#define MSG_MAX_SIZE 128

typedef struct _msg_block_ {
	int prev;
	int next;
	char origin[10];
	char msg[MSG_MAX_SIZE];
}msg_block;

typedef struct _mem_area_ {
	int head[3];
	int tail[3];
	msg_block msg_blocks[BLOCK_MAX_SIZE];
}mem_area;

//DECLARATIONS//
int init_mem_area(mem_area*);
int Block_Trans(mem_area*,int,int);
int send_message(mem_area*,int,char*,char*);
int receive_message(mem_area*,int,char*,char*);

//DEFINITIONS//
int init_mem_area(mem_area* MA)
{
	MA->head[0] = 0;
	MA->tail[0] = BLOCK_MAX_SIZE - 1;
	for (int i = 1; i < 3; i++)
	{
		MA->head[i] = -1;
		MA->tail[i] = -1;
	}
	for (int i = 0; i < BLOCK_MAX_SIZE; i++)
	{
		MA->msg_blocks[i].prev = i - 1;
		MA->msg_blocks[i].next = i + 1;
		strcpy(MA->msg_blocks[i].msg, "");
		strcpy(MA->msg_blocks[i].origin,"");
	}
	return 0;
}

int Block_Trans(mem_area* MA,int ori,int dest)
{
	if (MA->head[ori] == -1 && MA->tail[ori] == -1) return -1;
	if (MA->head[dest] == -1 && MA->tail[dest] == -1)
	{
		MA->head[dest] = MA->head[ori];
		MA->tail[dest] = MA->head[ori];
		MA->head[ori] = MA->msg_blocks[MA->head[ori]].next;
		MA->msg_blocks[MA->head[dest]].prev = -1;
		MA->msg_blocks[MA->tail[dest]].next = -1;
		MA->msg_blocks[MA->head[ori]].prev = -1;
	}
	else if (MA->head[ori] == MA->tail[ori])
	{
		MA->msg_blocks[MA->tail[dest]].next = MA->head[ori];
		MA->msg_blocks[MA->head[ori]].prev = MA->tail[dest];
		MA->tail[dest] = MA->head[ori];
		MA->head[ori] = MA->msg_blocks[MA->head[ori]].next;
		MA->tail[ori] = -1;
	}
	else
	{
		MA->msg_blocks[MA->tail[dest]].next = MA->head[ori];
		MA->msg_blocks[MA->head[ori]].prev = MA->tail[dest];
		MA->tail[dest] = MA->head[ori];
		MA->head[ori] = MA->msg_blocks[MA->head[ori]].next;
		MA->msg_blocks[MA->tail[dest]].next = -1;
		MA->msg_blocks[MA->head[ori]].prev = -1;
	}
	return 0;
}

int send_message(mem_area* MA, int path, char* message,char* origin)
{
	if(-1==Block_Trans(MA,0,path)) return -1;
	strcpy(MA->msg_blocks[MA->tail[path]].origin,origin);
	strcpy(MA->msg_blocks[MA->tail[path]].msg, message);
	return 0;
}

int receive_message(mem_area* MA, int path, char* message,char* origin)
{
	if(-1==Block_Trans(MA,path,0)) return -1;
	strcpy(message, MA->msg_blocks[MA->tail[0]].msg);
	strcpy(origin, MA->msg_blocks[MA->tail[0]].origin);
	strcpy(MA->msg_blocks[MA->tail[0]].msg, "");
	strcpy(MA->msg_blocks[MA->tail[0]].origin, "");
	return 0;
}

#endif
