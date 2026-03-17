#include<stdio.h>
#include<time.h>
#include<stdlib.h>
#include<string.h>

#define BLOCKSIZE 1024
#define SIZE 1024000
#define END 65535
#define FREE 0
#define ROOT_BLOCKNUM 2
#define MAX_OPEN_FILE 10
#define MAX_TEXT_SIZE 10000

int Recur_Remove = 0;
char* myStrtok;
char* mystrtok(char* s, const char* ct)
{
    char* sbegin, * send;
    sbegin = s ? s : myStrtok;
    if (!sbegin) {
        return NULL;
    }
    sbegin += strspn(sbegin, ct);
    if (*sbegin == '\0') {
        myStrtok = NULL;
        return (NULL);
    }
    send = strpbrk(sbegin, ct);
    if (send && *send != '\0')
        *send++ = '\0';
    myStrtok = send;
    return (sbegin);
}

typedef struct File_Control_Block
{
    char filename[8];//文件名
    char suffix[10];//扩展名
    unsigned char attribute;//文件属性
    char retainbyte[10];//10B保留字
    unsigned short time;//文件创建时间
    unsigned short date;//文件创建日期
    unsigned short start_block_no;//首块号
    unsigned long length;//文件大小
}FCB;

typedef struct File_Allocation_Table
{
    unsigned short id;
}FAT;

/*  7     6     5      4      3       2       1     0
  保留  保留  存档  子目录  卷标  系统文件  隐藏  只读*/

typedef struct USEROPEN
{
    char filename[8];//8B文件名
    char suffix[10];//3B扩展名
    unsigned char attribute;//属性
    char retainbyte[10];//10B保留字
    unsigned short time;//文件创建时间
    unsigned short date;//文件创建日期
    unsigned short start_block_no;//首块号
    unsigned long length;//文件大小

    char free;//表示目录项是否为空 0空1分配
    int father;//父目录的文件描述符
    int dirno;//相应打开文件的目录项在父目录文件中的盘块号
    int diroff;//相应打开文件的目录项在父目录文件的盘块中的目录项序号
    char dir[80];//相应打开文件所在的目录
    int count;//读写指针在文件中的位置
    char FCBstate;//修改标志位
    char topenfile;//表示该用户打开的表项是否为空，若值为0，表示为空
}useropen;

/*0号块*/
typedef struct BLOCK0
{
    char _info_[200];
    unsigned short root;
    unsigned char* startblock;
}block0;

/*全局变量定义*/
unsigned char* myvhard;//指向虚拟磁盘的起始地址
useropen OFL[MAX_OPEN_FILE];//用户打开文件表数组
useropen* ptr_cur_dir;//指向用户打开文件表中的当前目录的表项的位置
int curfd;//当前文件描述符(OFL[index])
char currentdir[80];//记录当前目录名
unsigned char* startp;//记录虚拟磁盘上数据区开始位置
char filename[] = "myfilesys";//虚拟空间保存路径
unsigned char buffer[SIZE];

/*函数声明*/
void startsys();
void my_format();
int my_cd(char* dirname);
void my_mkdir(char* dirname);
void my_rmdir(char* dirname);
void my_ls();
void my_create(char* filename);
void my_rm(char* filename);
int my_open(char* filename);
int my_close(int fd);
int my_write(int fd);
int do_write(int fd, char* text, int len, char W_Mode);
unsigned short get_free_block();
int my_read(int fd, int len);
int do_read(int fd, int len, char* text);
void my_exitsys();


/*文件系统初始化*/
/*
原型声明:		void startsys()
功能描述：		文件系统初始化,初始化所建立的文件系统
输入：无
输出：无

函数功能实现算法描述：
    1）申请磁盘空间
    2）打开系统磁盘，若不存在，创建新的系统磁盘，并格式化
    3）初始化用户打开文件表，将表项0分配给根目录文件使用
        并填写根目录文件的相关信息
    4）将ptr_cur_dir指向该用户打开文件表项
    5）将当前目录设置为根目录
*/
void startsys()
{
    FILE* fp;
    int i;
    myvhard = (unsigned char*)malloc(SIZE);
    memset(myvhard, 0, SIZE);
    fp = fopen(filename, "r");
    if (fp)
    {
        fread(buffer, SIZE, 1, fp);
        fclose(fp);
        if (buffer[0] == 0xaa)
        {
            printf("文件系统不存在，创建文件系统中……\n");
            my_format();
            printf("文件系统创建完毕!\n");
        }
        else
        {
            printf("检测到现有文件系统，正在读取……\n");
            for (i = 0; i < SIZE; i++)
                myvhard[i] = buffer[i];
            printf("文件系统读取完毕！\n");
        }
    }
    else
    {
        printf("文件系统不存在，创建文件系统中……\n");
        my_format();
        printf("文件系统创建完毕!\n");
    }


    strcpy(OFL[0].filename, "root");
    strcpy(OFL[0].suffix, "di");
    OFL[0].attribute = 0x2d;
    OFL[0].time = ((FCB*)(myvhard + 5 * BLOCKSIZE))->time;
    OFL[0].date = ((FCB*)(myvhard + 5 * BLOCKSIZE))->date;
    OFL[0].start_block_no = ((FCB*)(myvhard + 5 * BLOCKSIZE))->start_block_no;
    OFL[0].length = ((FCB*)(myvhard + 5 * BLOCKSIZE))->length;
    OFL[0].free = 1;
    OFL[0].dirno = 5;
    OFL[0].diroff = 0;
    OFL[0].count = 0;
    OFL[0].FCBstate = 0;
    OFL[0].topenfile = 0;
    OFL[0].father = 0;

    memset(currentdir, 0, sizeof(currentdir));
    strcpy(currentdir, "\\root\\");
    strcpy(OFL[0].dir, currentdir);
    startp = ((block0*)myvhard)->startblock;
    ptr_cur_dir = &OFL[0];
    curfd = 0;
}



/*
原型声明:		void my_format()
功能描述：		对虚拟磁盘进行格式化，布局虚拟磁盘，建立根目录文件
输入：无
输出：无

函数功能实现算法描述：
虚拟磁盘空间布局
1块		2块	   2块	995块
引导块	File_Allocation_Table1  File_Allocation_Table2 	数据区
虚拟磁盘一共划分成1000个磁盘块
每块1024个字节，磁盘空间布局如上
将数据区的第一块（即虚拟磁盘的第index=5块）分配给根目录文件
*/
void my_format()
{
    FILE* fp;
    FAT* FAT1, * FAT2;
    block0* b0;
    time_t* now;
    struct tm* nowtime;
    unsigned char* p;
    FCB* root;
    int i;

    p = myvhard;
    b0 = (block0*)p;
    FAT1 = (FAT*)(p + BLOCKSIZE);
    FAT2 = (FAT*)(p + 3 * BLOCKSIZE);

    int num = 0xaaaa;
    b0->root = 5;
    strcpy(b0->_info_, "Blocksize=1KB,Whole size=1000KB,Blocknum=1000,RootBlocknum=2\n");

    /*标记前7个已分配块的FAT表项*/
    for (int i = 0; i < 5; i++)
    {
        FAT1->id = END;
        FAT2->id = END;
        FAT1++; FAT2++;
    }

    FAT1->id = 6;
    FAT2->id = 6;
    FAT1++; FAT2++;

    FAT1->id = END;
    FAT2->id = END;
    FAT1++; FAT2++;

    /*将数据区的标记为空闲状态*/
    for (i = 7; i < SIZE / BLOCKSIZE; i++)
    {
        (*FAT1).id = FREE;
        (*FAT2).id = FREE;
        FAT1++;
        FAT2++;
    }
    /*
    创建根目录文件root，将数据区的第一块分配给根目录区
    在给磁盘上创建两个特殊的目录项：".",".."，
    除了文件名之外，其它都相同
    */
    p += BLOCKSIZE * 5;
    root = (FCB*)p;
    strcpy(root->filename, ".");
    strcpy(root->suffix, "di");
    root->attribute = 40;
    now = (time_t*)malloc(sizeof(time_t));
    time(now);
    nowtime = localtime(now);
    root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    root->start_block_no = 5;
    root->length = 2 * sizeof(FCB);
    root++;

    strcpy(root->filename, "..");
    strcpy(root->suffix, "di");
    root->attribute = 40;
    time(now);
    nowtime = localtime(now);
    root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    root->start_block_no = 5;
    root->length = 2 * sizeof(FCB);
    root++;

    fp = fopen(filename, "w");
    b0->startblock = p + BLOCKSIZE * 4;
    fwrite(myvhard, SIZE, 1, fp);
    free(now);
    fclose(fp);
}


/**/
/*
原型声明:		void my_exitsys()
功能描述：		退出文件系统
输入：			无
输出：			无

函数功能实现算法描述：

*/
void my_exitsys()
{
    FILE* fp;
    FCB* rootFCB;
    char text[MAX_TEXT_SIZE];
    while (curfd)
        curfd = my_close(curfd);
    if (OFL[curfd].FCBstate)
    {
        OFL[curfd].count = 0;
        do_read(curfd, OFL[curfd].length, text);
        rootFCB = (char*)text;
        rootFCB->length = OFL[curfd].length;
        OFL[curfd].count = 0;
        do_write(curfd, text, OFL[curfd].length, 2);
    }
    fp = fopen(filename, "w");
    fwrite(myvhard, SIZE, 1, fp);
    free(myvhard);
    fclose(fp);
}



/*
原型声明:		int do_read(int fd,int len,char *text)
功能描述：		实际读文件函数，读出指定文件中从指定指针开始的长度
                为len的内容到用户空间的text中
输入：
        fd		open（）函数的返回值，文件的描述符
        len		要求从文件中读出的字节数
        text	指向存放读出数据的用户区地址
输出：
    实际读出的字节数
函数功能实现算法描述：

*/
int do_read(int fd, int len, char* text)  //fd是文件描述符，len是要求从文件当中读取的字节数，读出数据的用户区地址
{
    unsigned char* buf;
    unsigned short Block_No;
    int off, i, lentmp;
    unsigned char* bkptr;
    char* txtmp, * p;
    FAT* FAT1, * FATptr;
    FAT1 = (FAT*)(myvhard + BLOCKSIZE);  //myvhard是虚拟磁盘的起始位置
    lentmp = len;
    txtmp = text;


    //申请1024B空间作为缓冲区buffer
    buf = (unsigned char*)malloc(1024);
    if (buf == NULL)
    {
        printf("malloc failed!\n");
        return -1;
    }


    //将读写指针转换为块内偏移量off
    off = OFL[fd].count;  //count是读写指针在文件中的位置


    //利用打开文件表表项中的首块号查找File_Allocation_Table表，找到该逻辑所在的磁盘块号，将该磁盘块号转化为虚拟磁盘上的内存位置
    Block_No = OFL[fd].start_block_no;  //将文件首块号赋值给Block_No
    FATptr = FAT1 + Block_No;
    while (off >= BLOCKSIZE)  //找到该逻辑块所在的盘块号
    {
        off = off - BLOCKSIZE;
        Block_No = FATptr->id;
        FATptr = FAT1 + Block_No;
        if (Block_No == END)
        {
            printf("Error,the block is not exist.\n");
            return -1;
        }
    }
    bkptr = (unsigned char*)(myvhard + Block_No * BLOCKSIZE);  //将该磁盘块号转换为虚拟磁盘上的内存位置



    //将内存位置开始的1024B（一个盘块大小）读入到buf中
    for (i = 0; i < BLOCKSIZE; i++)
    {
        buf[i] = bkptr[i];
    }


    while (len > 0)
    {
        if (BLOCKSIZE - off > len)
        {
            for (p = buf + off; len > 0; p++, txtmp++)
            {
                *txtmp = *p;
                len--;
                off++;
                OFL[fd].count++;  //读写指针也增加
            }
        }
        else
        {
            for (p = buf + off; p < buf + BLOCKSIZE; p++, txtmp++)
            {
                *txtmp = *p;
                len--;
                OFL[fd].count++;
            }
            off = 0;
            Block_No = FATptr->id;
            FATptr = FAT1 + Block_No;
            bkptr = (unsigned char*)(myvhard + Block_No * BLOCKSIZE);
            //strncpy(buf,bkptr,BLOCKSIZE);
            for (i = 0; i < BLOCKSIZE; i++)
            {
                buf[i] = bkptr[i];
            }
        }
    }
    free(buf);
    return lentmp - len;
}

/**/
/*
原型声明:		int my_read(int fd,int len)
功能描述：		读文件函数
输入：
        fd		打开文件的id
        len		要读出字符的个数
输出：			返回实际读的字符的个数

函数功能实现算法描述：

*/
int my_read(int fd, int len)  //fd是文件描述符（可以理解为文件的编号，len是指要读取的长度）
{
    char text[MAX_TEXT_SIZE];

    //如果文件描述符超出最大打开数就报错
    if (fd > MAX_OPEN_FILE)
    {
        printf("The File is not exist!\n");
        return -1;
    }
    OFL[curfd].count = 0;  //让读写指针位于文件头部

    if (do_read(fd, len, text) > 0)
    {
        text[len] = '\0';
        printf("%s\n", text);
    }
    else
    {
        printf("Read Error!\n");
        return -1;
    }
    return len;
}


/**/
/*
原型声明:		int do_write(int fd,char *text,int len,char W_Mode)
功能描述：		实际写文件函数
输入：
        fd		当前打开的文件的id
        text	指向要写入的内容的指针
        len		本次要写入字节数
        W_Mode	写方式
输出：			实际写入的字节数

函数功能实现算法描述：

*/
int do_write(int fd, char* text, int len, char W_Mode)
{
    unsigned char* buf;
    unsigned short Block_No;
    int off, tmplen = 0, tmplen2 = 0, i, rwptr;
    unsigned char* bkptr, * p;
    char* txtmp, flag = 0;
    FAT* FAT1, * FATptr;
    FAT1 = (FAT*)(myvhard + BLOCKSIZE);
    txtmp = text;
    buf = (unsigned char*)malloc(BLOCKSIZE);
    if (buf == NULL)
    {
        printf("malloc failed!\n");
        return -1;
    }

    rwptr = off = OFL[fd].count;
    Block_No = OFL[fd].start_block_no;
    FATptr = FAT1 + Block_No;
    while (off >= BLOCKSIZE)
    {
        off = off - BLOCKSIZE;
        Block_No = FATptr->id;
        if (Block_No == END)
        {
            Block_No = FATptr->id = get_free_block();
            if (Block_No == END) return -1;
            FATptr = FAT1 + Block_No;
            FATptr->id = END;
        }
        FATptr = FAT1 + Block_No;
    }

    FATptr->id = END;
    bkptr = (unsigned char*)(myvhard + Block_No * BLOCKSIZE);
    while (tmplen < len)
    {
        for (i = 0; i < BLOCKSIZE; i++)
        {
            buf[i] = 0;
        }
        for (i = 0; i < BLOCKSIZE; i++)
        {
            buf[i] = bkptr[i];
            tmplen2++;
            if (tmplen2 == OFL[curfd].length)
                break;
        }

        for (p = buf + off; p < buf + BLOCKSIZE; p++)
        {
            *p = *txtmp;
            tmplen++;
            txtmp++;
            off++;
            if (tmplen == len)
                break;
        }

        for (i = 0; i < BLOCKSIZE; i++)
        {
            bkptr[i] = buf[i];
        }
        OFL[fd].count = rwptr + tmplen;
        if (off >= BLOCKSIZE)
        {
            off = off - BLOCKSIZE;
            Block_No = FATptr->id;
            if (Block_No == END)
            {
                Block_No = FATptr->id = get_free_block();
                if (Block_No == END) return -1;
                FATptr = FAT1 + Block_No;
                FATptr->id = END;
            }
            FATptr = FAT1 + Block_No;
            bkptr = (unsigned char*)(myvhard + Block_No * BLOCKSIZE);
        }
    }
    free(buf);
    if (OFL[fd].count > OFL[fd].length)
    {
        OFL[fd].length = OFL[fd].count;
    }
    OFL[fd].FCBstate = 1;
    return tmplen;
}

/*
原型声明:		unsigned short get_free_block()
功能描述：		寻找下一个空闲盘块
输入：			无
输出：			返回空闲盘块的id

函数功能实现算法描述：
*/

unsigned short get_free_block()
{
    unsigned short i;
    FAT* FAT1, * FATptr;

    FAT1 = (FAT*)(myvhard + BLOCKSIZE);
    for (i = 6; i < END; i++)
    {
        FATptr = FAT1 + i;
        if (FATptr->id == FREE)
        {
            return i;
        }
    }
    printf("Error,Can't find free block!\n");
    return END;
}

/*
原型声明:		int get_free_fd()
功能描述：		寻找空闲文件表项
输入：			无
输出：			返回空闲文件表项的id

函数功能实现算法描述：

*/
int get_free_fd()
{
    int i;
    for (i = 0; i < MAX_OPEN_FILE; i++)
    {
        if (OFL[i].free == 0)
        {
            return i;
        }
    }
    printf("Error,open too many files!\n");
    return -1;
}
/**/
/*
原型声明:		int my_write(int fd)
功能描述：		写文件函数
输入：
        fd		打开文件的id
输出：			返回实际写的长度

函数功能实现算法描述：

*/
int my_write(int fd)
{
    int W_Mode = 0, wlen = 0;
    FAT* FAT1, * FATptr;
    unsigned short Block_No;
    unsigned char* bkptr;
    char text[MAX_TEXT_SIZE];
    FAT1 = (FAT*)(myvhard + BLOCKSIZE);
    if (fd > MAX_OPEN_FILE)
    {
        printf("The file is not exist!\n");
        return -1;
    }
    while (W_Mode < 1 || W_Mode>3)
    {
        printf("Please enter the number of write style:\n1.cut write\t2.cover write\t3.add write\n");
        scanf("%d", &W_Mode);
        getchar();
        switch (W_Mode)
        {
        case 1://截断写
        {
            Block_No = OFL[fd].start_block_no;
            FATptr = FAT1 + Block_No;
            while (FATptr->id != END)
            {
                Block_No = FATptr->id;
                FATptr->id = FREE;
                FATptr = FAT1 + Block_No;
            }
            FATptr->id = FREE;
            Block_No = OFL[fd].start_block_no;
            FATptr = FAT1 + Block_No;
            FATptr->id = END;
            OFL[fd].length = 0;
            OFL[fd].count = 0;
            break;
        }
        case 2://覆盖写
        {
            OFL[fd].count = 0;
            break;
        }
        case 3://追加写
        {
            Block_No = OFL[fd].start_block_no;
            FATptr = FAT1 + Block_No;
            OFL[fd].count = 0;
            while (FATptr->id != END)
            {
                Block_No = FATptr->id;
                FATptr = FAT1 + Block_No;
                OFL[fd].count += BLOCKSIZE;
            }
            bkptr = (unsigned char*)(myvhard + Block_No * BLOCKSIZE);
            while (*bkptr != 0)
            {
                bkptr++;
                OFL[fd].count++;
            }
            break;
        }
        default:
            break;
        }
    }
    printf("please input write data:\n");
    strcpy(text, "");
    gets(text);
    if (do_write(fd, text, strlen(text), W_Mode) > 0)
    {
        wlen += strlen(text);
    }
    else
    {
        return -1;
    }
    if (OFL[fd].count > OFL[fd].length)
    {
        OFL[fd].length = OFL[fd].count;
    }
    OFL[fd].FCBstate = 1;
    return wlen;
}


/*
原型声明:		void my_mkdir(char *dirname)
功能描述：		创建子目录函数,在当前目录下创建名为dirname的目录
输入：
        dirname		指向新建目录的名字的指针
输出：无

函数功能实现算法描述：

*/
void my_mkdir(char* dirname)//创建子目录
{
    /*if (strcmp(dirname, "cc") == 0)
        printf("I am here!\n");*/
    FCB* dirFCB, * ptr_FCBtmp;/*指向当前位置的FCB*/
    int rbn, i, fd;//rbn(read byte num);
    unsigned short Block_No;
    char text[MAX_TEXT_SIZE], * p;
    time_t* now;
    struct tm* nowtime;
    /*
    将当前的文件信息读到text中
    rbn 是实际读取的字节数
    */
    OFL[curfd].count = 0;
    rbn = do_read(curfd, OFL[curfd].length, text); //将当前目录文件读入到内存中
    dirFCB = (FCB*)text;

    //检测是否有相同的目录名
    for (i = 0; i < rbn / sizeof(FCB); i++)
    {
        if (strcmp(dirname, dirFCB->filename) == 0)
        {
            printf("Error,the dirname is already exist!\n");
            return;
        }
        dirFCB++;
    }

    dirFCB = (FCB*)text;
    /*让dirFCB指向第一个空闲位置*/
    for (i = 0; i < rbn / sizeof(FCB); i++)
    {
        if (strcmp(dirFCB->filename, "") == 0)
            break;
        dirFCB++;
    }
    OFL[curfd].count = i * sizeof(FCB);//表示当前文件读写指针指向什么位置。

    //寻找一个空闲文件表项
    fd = get_free_fd();
    if (fd < 0)
    {
        return;
    }


    //寻找空闲盘块    
    Block_No = get_free_block();
    if (Block_No == END)
    {
        //回收文件占据的用户打开表表项
        OFL[fd].attribute = 0;
        OFL[fd].count = 0;
        OFL[fd].date = 0;
        strcpy(OFL[fd].dir, "");
        strcpy(OFL[fd].filename, "");
        strcpy(OFL[fd].suffix, "");
        OFL[fd].length = 0;
        OFL[fd].time = 0;
        OFL[fd].free = 0;
        OFL[fd].topenfile = 0;

        return -1;
    }

    ptr_FCBtmp = (FCB*)malloc(sizeof(FCB));
    now = (time_t*)malloc(sizeof(time_t));

    //在当前目录下新建目录项
    ptr_FCBtmp->attribute = 0x30;  //0x30在ASCII码上代表0，即表示文件属性为0（目录文件）

    //获取时间，调整时间格式
    time(now);
    nowtime = localtime(now);
    ptr_FCBtmp->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    ptr_FCBtmp->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;


    strcpy(ptr_FCBtmp->filename, dirname);
    strcpy(ptr_FCBtmp->suffix, "di");
    ptr_FCBtmp->start_block_no = Block_No;
    ptr_FCBtmp->length = 2 * sizeof(FCB);

    OFL[fd].attribute = ptr_FCBtmp->attribute;
    OFL[fd].count = 0;
    OFL[fd].date = ptr_FCBtmp->date;
    strcpy(OFL[fd].dir, OFL[curfd].dir);

    p = OFL[fd].dir;
    while (*p != '\0')
        p++;
    strcpy(p, dirname);
    while (*p != '\0') p++;
    *p = '\\'; p++;
    *p = '\0';

    OFL[fd].dirno = OFL[curfd].start_block_no;
    OFL[fd].diroff = i;
    strcpy(OFL[fd].suffix, ptr_FCBtmp->suffix);
    strcpy(OFL[fd].filename, ptr_FCBtmp->filename);
    OFL[fd].FCBstate = 1;
    OFL[fd].start_block_no = ptr_FCBtmp->start_block_no;
    OFL[fd].length = ptr_FCBtmp->length;
    OFL[fd].free = 1;
    OFL[fd].time = ptr_FCBtmp->time;
    OFL[fd].topenfile = 1;

    do_write(curfd, (char*)ptr_FCBtmp, sizeof(FCB), 2);

    ptr_FCBtmp->attribute = 0x28;
    time(now);
    nowtime = localtime(now);
    ptr_FCBtmp->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    ptr_FCBtmp->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    strcpy(ptr_FCBtmp->filename, ".");
    strcpy(ptr_FCBtmp->suffix, "di");
    ptr_FCBtmp->start_block_no = Block_No;
    ptr_FCBtmp->length = 2 * sizeof(FCB);

    do_write(fd, (char*)ptr_FCBtmp, sizeof(FCB), 2);

    ptr_FCBtmp->attribute = 0x28;
    time(now);
    nowtime = localtime(now);
    ptr_FCBtmp->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    ptr_FCBtmp->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    strcpy(ptr_FCBtmp->filename, "..");
    strcpy(ptr_FCBtmp->suffix, "di");
    ptr_FCBtmp->start_block_no = OFL[curfd].start_block_no;
    ptr_FCBtmp->length = OFL[curfd].length;

    do_write(fd, (char*)ptr_FCBtmp, sizeof(FCB), 2);

    OFL[curfd].count = 0;
    do_read(curfd, OFL[curfd].length, text);

    ptr_FCBtmp = (FCB*)text;
    ptr_FCBtmp->length = OFL[curfd].length;
    my_close(fd);

    OFL[curfd].count = 0;

    do_write(curfd, text, ptr_FCBtmp->length, 2);

}

/*
原型声明:		void my_ls()
功能描述：		显示目录函数
输入：			无
输出：			无

函数功能实现算法描述：
*/
void my_ls()
{
    FCB* FCBptr;
    int i;
    char text[MAX_TEXT_SIZE];
    unsigned short Block_No;
    OFL[curfd].count = 0;
    do_read(curfd, OFL[curfd].length, text);
    FCBptr = (FCB*)text;
    for (i = 0; i < (int)(OFL[curfd].length / sizeof(FCB)); i++)
    {
        if (strcmp(FCBptr->filename, ".") == 0 || strcmp(FCBptr->filename, "..") == 0)
        {
            FCBptr++;
            continue;
        }
        else if (FCBptr->filename[0] != '\0')
        {
            if (FCBptr->attribute & 0x20)
            {
                printf("%s\\\t\t<DIR>\t\t%d/%d/%d\t%02d:%02d:%02d\n", FCBptr->filename, ((FCBptr->date) >> 9) + 1980, ((FCBptr->date) >> 5) & 0x000f, (FCBptr->date) & 0x001f, FCBptr->time >> 11, (FCBptr->time >> 5) & 0x003f, FCBptr->time & 0x001f * 2);
            }
            else
            {
                printf("%s.%s\t\t%dB\t\t%d/%d/%d\t%02d:%02d:%02d\t\n", FCBptr->filename, FCBptr->suffix, FCBptr->length, ((FCBptr->date) >> 9) + 1980, (FCBptr->date >> 5) & 0x000f, FCBptr->date & 0x1f, FCBptr->time >> 11, (FCBptr->time >> 5) & 0x3f, FCBptr->time & 0x1f * 2);
            }
        }
        FCBptr++;
    }
    OFL[curfd].count = 0;
}

/*
原型声明:		void my_rmdir(char *dirname)
功能描述：		删除子目录函数
输入：
        dirname		指向新建目录的名字的指针
输出：无

函数功能实现算法描述：

*/
void my_rmdir(char* dirname)
{
    int rbn, fd;
    char text[MAX_TEXT_SIZE];
    FCB* FCBptr, * FCBtmp, * FCBtmp2;
    unsigned short Block_No;
    int i, j;
    FAT* FAT1, * FATptr;

    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)
    {
        printf("Error,can't remove default directory.\n");
        return;
    }

    FAT1 = (FAT*)(myvhard + BLOCKSIZE);
    OFL[curfd].count = 0;
    //
    rbn = do_read(curfd, OFL[curfd].length, text);

    FCBptr = (FCB*)text;
    for (i = 0; i < rbn / sizeof(FCB); i++)
    {
        if (strcmp(dirname, FCBptr->filename) == 0)
        {
            break;
        }
        FCBptr++;
    }
    if (i >= rbn / sizeof(FCB))
    {
        printf("Error,the directory is not exist.\n");
        return;
    }
    Block_No = FCBptr->start_block_no;
    FCBtmp2 = FCBtmp = (FCB*)(myvhard + Block_No * BLOCKSIZE);
    for (j = 0; j < FCBtmp->length / sizeof(FCB); j++)
    {
        //递归删除非空目录
        if (strcmp(FCBtmp2->filename, ".") && strcmp(FCBtmp2->filename, "..") && FCBtmp2->filename[0] != '\0')
        {
            if (Recur_Remove == 0)
            {
                printf("目录%s不为空，要删除其下所有子目录和文件吗？[输入y以确认，输入其他字符取消]\n", FCBptr->filename);
                char temp[2];
                gets(temp);
                if (strcmp(temp, "y") == 0)
                {
                    Recur_Remove = 1;
                }
                else
                {
                    return;
                }
            }
            if (strcmp(FCBtmp2->suffix, "di") == 0)
            {
                //删除子目录
                int tempfd = my_cd(FCBptr->filename);
                my_rmdir((((FCB*)(myvhard + OFL[tempfd].start_block_no * BLOCKSIZE)) + j)->filename);
                my_cd("..");
                FCBtmp2++;
            }
            else
            {
                //删除子文件
                int tempfd = my_cd(FCBptr->filename);
                char tmp_filename[20];
                strcpy(tmp_filename, (((FCB*)(myvhard + OFL[tempfd].start_block_no * BLOCKSIZE)) + j)->filename);
                strcat(tmp_filename, ".");
                char tmp_suffix[10];
                strcpy(tmp_suffix, (((FCB*)(myvhard + OFL[tempfd].start_block_no * BLOCKSIZE)) + j)->suffix);
                my_rm(strcat(tmp_filename, tmp_suffix));
                my_cd("..");
                FCBtmp2++;
            }
        }
        else FCBtmp2++;
    }

    while (Block_No != END)
    {
        FATptr = FAT1 + Block_No;
        Block_No = FATptr->id;
        FATptr->id = FREE;
    }

    strcpy(FCBptr->filename, "");
    strcpy(FCBptr->suffix, "");
    FCBptr->start_block_no = END;
    OFL[curfd].count = 0;
    do_write(curfd, text, OFL[curfd].length, 2);
}


/*
原型声明:		int my_open(char *filename)
功能描述：		打开文件函数
输入：
        filename		指向要打开的文件的名字的指针
输出：			返回打开的文件的id

函数功能实现算法描述：

*/
int my_open(char* filename)
{
    int i, fd, rbn;
    char text[MAX_TEXT_SIZE], * p, * fname, * suffix;
    FCB* FCBptr;
    char suffixd = 0;
    fname = strtok(filename, ".");  //提取出文件名
    suffix = strtok(NULL, ".");  //提取出后缀名
    if (!suffix)
    {
        suffix = (char*)malloc(3);
        strcpy(suffix, "di");
        suffixd = 1;
    }
    for (i = 0; i < MAX_OPEN_FILE; i++)
    {
        //匹配文件名和后缀名，如果i==curfd则未打开
        if (strcmp(OFL[i].filename, filename) == 0 && strcmp(OFL[i].suffix, suffix) == 0 && i != curfd)
        {
            if (strcmp(suffix, "di") == 0)
                printf("Error,the DIR is already open.\n");
            else
                printf("Error,the file is already open.\n");
            return -1;
        }
    }
    OFL[curfd].count = 0;
    rbn = do_read(curfd, OFL[curfd].length, text);  //父目录文件的内容读取到内存
    FCBptr = (FCB*)text;

    //在目录文件内容中寻找是否存在该文件
    for (i = 0; i < rbn / sizeof(FCB); i++)
    {
        if (strcmp(filename, FCBptr->filename) == 0 && strcmp(FCBptr->suffix, suffix) == 0)
        {
            break;
        }
        FCBptr++;
    }
    if (i >= rbn / sizeof(FCB))
    {
        if (strcmp(suffix, "di") == 0)
            printf("Error,the DIR is not exist.\n");
        else
            printf("Error,the file is not exist.\n");

        return curfd;
    }


    if (suffixd)
    {
        free(suffix);
    }

    //寻找用户打开文件表中是否有空表项，有就为想要打开的文件分配一个空表项
    fd = get_free_fd();
    if (fd == -1)
    {
        return -1;
    }

    //将文件名文件后缀名赋值给空表项的对应属性
    strcpy(OFL[fd].filename, FCBptr->filename);
    strcpy(OFL[fd].suffix, FCBptr->suffix);
    OFL[fd].attribute = FCBptr->attribute;
    OFL[fd].count = 0;
    OFL[fd].date = FCBptr->date;
    OFL[fd].start_block_no = FCBptr->start_block_no;
    OFL[fd].length = FCBptr->length;
    OFL[fd].time = FCBptr->time;
    OFL[fd].father = curfd;
    OFL[fd].dirno = OFL[curfd].start_block_no;
    OFL[fd].diroff = i;
    OFL[fd].FCBstate = 0;
    OFL[fd].free = 1;
    OFL[fd].topenfile = 1;  //将打开表项置为占用状态
    strcpy(OFL[fd].dir, OFL[curfd].dir);
    p = OFL[fd].dir;

    while (*p != '\0')
        p++;
    strcpy(p, filename);
    while (*p != '\0') p++;
    if (OFL[fd].attribute & 0x20) //如果是目录文件，0x20是100000
    {
        *p = '\\';
        p++;
        *p = '\0';

    }
    else
    {
        *p = '.';
        p++;
        strcpy(p, OFL[fd].suffix);
    }

    return fd;
}

/**/
/*
原型声明:		int my_close(int fd)
功能描述：		关闭文件函数
输入：
        fd		打开文件的id
输出：			返回fd的father的id

函数功能实现算法描述：

*/
int my_close(int fd)
{
    FCB* faFCB;
    char text[MAX_TEXT_SIZE];
    int fa;

    //检查fd的有效性
    if (fd > MAX_OPEN_FILE || fd <= 0)
    {
        printf("Error,the file is not exist.\n");
        return -1;
    }

    fa = OFL[fd].father;
    //printf("OFL[fd].father = %d", OFL[fd].father);

    if (OFL[fd].FCBstate)  //如果FCBstate值为1，则将File_Control_Block的内容保存到虚拟磁盘空间上该文件的目录项中
    {
        fa = OFL[fd].father;
        OFL[fa].count = 0;
        do_read(fa, OFL[fa].length, text);
        //将父目录文件的对应磁盘块的内容给覆盖写掉
        faFCB = (FCB*)(text + sizeof(FCB) * OFL[fd].diroff);
        faFCB->attribute = OFL[fd].attribute;
        faFCB->date = OFL[fd].date;
        faFCB->start_block_no = OFL[fd].start_block_no;
        faFCB->length = OFL[fd].length;
        faFCB->time = OFL[fd].time;
        strcpy(faFCB->filename, OFL[fd].filename);
        strcpy(faFCB->suffix, OFL[fd].suffix);

        OFL[fa].count = 0;
        do_write(fa, text, OFL[fa].length, 2);
    }

    //回收文件占据的用户打开表表项
    OFL[fd].attribute = 0;
    OFL[fd].count = 0;
    OFL[fd].date = 0;
    strcpy(OFL[fd].dir, "");
    strcpy(OFL[fd].filename, "");
    strcpy(OFL[fd].suffix, "");
    OFL[fd].length = 0;
    OFL[fd].time = 0;
    OFL[fd].free = 0;
    OFL[fd].topenfile = 0;

    return fa;
}

/*
原型声明:		void my_cd(char *dirname)
功能描述：		更改当前目录函数
输入：
    dirname		指向目录名的指针
输出：			无

函数功能实现算法描述：

*/
int my_cd(char* dirname)
{
    char* p, text[MAX_TEXT_SIZE];
    char* index = NULL;
    int fd, i;
    /*char _dirname_strtok[100];
    strcpy(_dirname_strtok, dirname);*/
    char backupdirname[20];
    strcpy(backupdirname, dirname);
    p = mystrtok(dirname, "\\");  //提取出cd的命令
    //index = p + strlen(p) + 1;
    if (strcmp(p, ".") == 0)   //emmm什么都不干~~
        return curfd;
    if (strcmp(p, "..") == 0)  //cd .. 去往上层目录
    {
        fd = OFL[curfd].father;
        my_close(curfd);//关闭当前目录。
        curfd = fd;//当前文件标识符设为当前目录父亲的
        ptr_cur_dir = &OFL[curfd];//当前目录指针指向关闭目录的父亲对应的OFL表项。
        return curfd;
    }
    if (strcmp(p, "root") == 0)
    {
        /*关闭除了OFL[0](root)外的所有打开项*/
        //顺序不饿能颠倒，否则会出现磁盘块覆盖顺序错误。
        for (i = MAX_OPEN_FILE - 1; i >= 1; i--)
        {
            if (OFL[i].free)
                my_close(i);
        }
        //当前打开指针和文件标识符号指向root
        ptr_cur_dir = &OFL[0];
        curfd = 0;
        p = strtok(NULL, "\\");
        return curfd;
    }

    while (p)  //设置当前目录为该目录
    {
        /*if (*(p + strlen(p) + 1) == '\0') break;
        index = p + strlen(p) + 1;*/
        fd = my_open(p);//不断向子目录Open
        if (fd > 0)
        {
            ptr_cur_dir = &OFL[fd];
            curfd = fd;
        }
        else
            return;
        p = mystrtok(NULL, "\\");//注意，strtok不能嵌套使用
    }
    return curfd;
}


/*
原型声明:		void my_create(char *filename)
功能描述：		创建文件函数
输入：			filename	指向文件名的指针
输出：			无

函数功能实现算法描述：
*/
void my_create(char* filename)
{
    char* fname, * suffix, text[MAX_TEXT_SIZE];
    int fd, rbn, i;
    FCB* fileFCB, * FCBtmp;
    time_t* now;
    struct tm* nowtime;
    unsigned short Block_No;
    FAT* FAT1, * FATptr;

    FAT1 = (FAT*)(myvhard + BLOCKSIZE);
    fname = strtok(filename, ".");
    suffix = strtok(NULL, ".");
    if (strcmp(fname, "") == 0)
    {
        printf("Error,creating file must have a right name.\n");
        return;
    }
    if (!suffix)
    {
        printf("Error,creating file must have a suffix.\n");
        return;
    }

    OFL[curfd].count = 0;
    rbn = do_read(curfd, OFL[curfd].length, text);
    fileFCB = (FCB*)text;
    for (i = 0; i < rbn / sizeof(FCB); i++)
    {
        if (strcmp(fname, fileFCB->filename) == 0 && strcmp(suffix, fileFCB->suffix) == 0)
        {
            printf("Error,the filename is already exist!\n");
            return;
        }
        fileFCB++;
    }

    fileFCB = (FCB*)text;
    for (i = 0; i < rbn / sizeof(FCB); i++)
    {
        if (strcmp(fileFCB->filename, "") == 0)
            break;
        fileFCB++;
    }
    OFL[curfd].count = i * sizeof(FCB);

    Block_No = get_free_block();
    if (Block_No == END)
    {
        return;
    }
    FCBtmp = (FCB*)malloc(sizeof(FCB));
    now = (time_t*)malloc(sizeof(time_t));

    FCBtmp->attribute = 0x00;
    time(now);
    nowtime = localtime(now);
    FCBtmp->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    FCBtmp->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    strcpy(FCBtmp->filename, fname);
    strcpy(FCBtmp->suffix, suffix);
    FCBtmp->start_block_no = Block_No;
    FCBtmp->length = 0;

    do_write(curfd, (char*)FCBtmp, sizeof(FCB), 2);
    free(FCBtmp);
    free(now);
    OFL[curfd].count = 0;
    do_read(curfd, OFL[curfd].length, text);
    FCBtmp = (FCB*)text;
    FCBtmp->length = OFL[curfd].length;
    OFL[curfd].count = 0;
    do_write(curfd, text, OFL[curfd].length, 2);
    OFL[curfd].FCBstate = 1;
    FATptr = (FAT*)(FAT1 + Block_No);
    FATptr->id = END;
}

/*
原型声明:		void my_rm(char *filename)
功能描述：		删除文件函数
输入：			filename	指向文件名的指针
输出：			无

函数功能实现算法描述：

*/
void my_rm(char* filename)
{
    char* fname, * suffix;
    char text[MAX_TEXT_SIZE];
    FCB* FCBptr;
    int i, rbn;
    unsigned short Block_No;
    FAT* FAT1, * FATptr;

    FAT1 = (FAT*)(myvhard + BLOCKSIZE);
    fname = strtok(filename, ".");
    suffix = strtok(NULL, ".");
    if (!fname || strcmp(fname, "") == 0)
    {
        printf("Error,removing file must have a right name.\n");
        return;
    }
    if (!suffix)
    {
        printf("Error,removing file must have a extern name.\n");
        return;
    }
    OFL[curfd].count = 0;
    rbn = do_read(curfd, OFL[curfd].length, text);
    FCBptr = (FCB*)text;
    for (i = 0; i < rbn / sizeof(FCB); i++)
    {
        if (strcmp(fname, FCBptr->filename) == 0 && strcmp(suffix, FCBptr->suffix) == 0)
        {
            break;
        }
        FCBptr++;
    }
    if (i >= rbn / sizeof(FCB))
    {
        printf("Error,the file is not exist.\n");
        return;
    }

    Block_No = FCBptr->start_block_no;
    while (Block_No != END)
    {
        FATptr = FAT1 + Block_No;
        Block_No = FATptr->id;
        FATptr->id = FREE;
    }

    strcpy(FCBptr->filename, "");
    strcpy(FCBptr->suffix, "");
    FCBptr->start_block_no = END;
    OFL[curfd].count = 0;
    do_write(curfd, text, OFL[curfd].length, 2);
}

int main()
{
    printf("%d\n", sizeof(FCB));
    char cmd[15][10] = { "mkdir","rmdir","ls","cd","create","rm","open","close","write","read","exit","clear" };
    char s[120], * sp;
    int cmdn, i;
    startsys();
part1:
    printf("========================文件系统管理台===============================\n\n");
    printf("Command:\targ:\t\t\thelp:\n\n");
    printf("cd\t\t目录名/路径名\t\t切换当前目录到指定目录\n");
    printf("mkdir\t\t目录名\t\t\t在当前目录创建新目录\n");
    printf("rmdir\t\t目录名\t\t\t在当前目录删除指定目录\n");
    printf("ls\t\t无\t\t\t显示当前目录下的目录和文件\n");
    printf("create\t\t文件名\t\t\t在当前目录下创建指定文件\n");
    printf("rm\t\t文件名\t\t\t在当前目录下删除指定文件\n");
    printf("open\t\t文件名\t\t\t在当前目录下打开指定文件\n");
    printf("write\t\t无\t\t\t在打开文件状态下，写该文件\n");
    printf("read\t\t无\t\t\t在打开文件状态下，读取该文件\n");
    printf("close\t\t无\t\t\t在打开文件状态下，读取该文件\n");
    printf("exit\t\t无\t\t\t退出系统\n\n");
    printf("clear\t\t无\t\t\t清屏\n\n");
    printf("=====================================================================\n\n");

    while (1)
    {
        printf("%s>", OFL[curfd].dir);
        gets(s);
        cmdn = -1;
        if (strcmp(s, ""))
        {
            sp = strtok(s, " ");
            for (i = 0; i < 15; i++)
            {
                if (strcmp(sp, cmd[i]) == 0)
                {
                    cmdn = i;
                    break;
                }
            }
            switch (cmdn)
            {
            case 0:
                sp = strtok(NULL, " ");
                if (sp && OFL[curfd].attribute & 0x20)
                    my_mkdir(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 1:
                sp = strtok(NULL, " ");
                if (sp && OFL[curfd].attribute & 0x20)
                {
                    my_rmdir(sp);
                    if (Recur_Remove == 1)
                    {
                        puts("删除成功！");
                        Recur_Remove = 0;
                    }
                }
                else
                    printf("Please input the right command.\n");
                break;
            case 2:
                if (OFL[curfd].attribute & 0x20)
                {
                    my_ls();
                    putchar('\n');
                }
                else
                    printf("Please input the right command.\n");
                break;
            case 3:
                sp = strtok(NULL, " ");
                if (sp && OFL[curfd].attribute & 0x20)
                    my_cd(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 4:
                sp = strtok(NULL, " ");
                if (sp && OFL[curfd].attribute & 0x20)
                    my_create(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 5:
                sp = strtok(NULL, " ");
                if (sp && OFL[curfd].attribute & 0x20)
                    my_rm(sp);
                else
                    printf("Please input the right command.\n");
                break;
            case 6:
                sp = strtok(NULL, " ");
                if (sp && OFL[curfd].attribute & 0x20)
                {
                    if (strchr(sp, '.'))
                        curfd = my_open(sp);
                    else
                        printf("the openfile should have suffix.\n");
                }
                else
                    printf("Please input the right command.\n");
                break;
            case 7:
                if (!(OFL[curfd].attribute & 0x20))
                    curfd = my_close(curfd);
                else
                    printf("No files opened.\n");
                break;
            case 8:
                if (!(OFL[curfd].attribute & 0x20))
                    my_write(curfd);
                else
                    printf("No files opened.\n");
                break;
            case 9:
                if (!(OFL[curfd].attribute & 0x20))
                    my_read(curfd, OFL[curfd].length);
                else
                    printf("No files opened.\n");
                break;
            case 10:
                if (OFL[curfd].attribute & 0x20)
                {
                    my_exitsys();
                    return;
                }
                else
                    printf("Please input the right command.\n");
                break;
            case 11:
                system("cls");
                goto part1;
                break;
            default:
                printf("Please input the right command.\n");
                break;
            }
        }
    }
    return 0;
}
