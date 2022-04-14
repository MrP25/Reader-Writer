#include<iostream>
#include<string>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <io.h>
#include <string.h>
#include<algorithm>
#include<Windows.h>
#include<process.h>
using namespace std;

#define READER 'R'                   //读者
#define WRITER 'W'                   //写者
#define INTE_PER_SEC 1000            //时间倍数
#define MAX_THREAD_NUM 50            //最大线程数目

//变量声明初始化
int readercount = 0;    //等待的读者数目
int writercount = 0;    //等待的写者数目
int t = 0;  //time
HANDLE mutex2;      //
HANDLE mutex;       //
HANDLE mutex3;       //

HANDLE wmutex;      //写者互斥信号量 保证只有一个写者
HANDLE rmutex;      //读者信号量 保证读的时候没有人写

struct thread_info {
    int id;		    //线程id
    char status;    //线程状态R/W
    double start;   //线程开始时间
    double delay;	//线程操作时间
};

/*读者优先*/

// 读者优先 读者线程
void rp_Reader(void* p)
{    
    int m_id;
    DWORD m_start;
    DWORD m_delay;

    //从参数中获得信息
    m_id = ((thread_info*)(p))->id;
    m_start = (DWORD)(((thread_info*)(p))->start * INTE_PER_SEC);
    m_delay = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);

    Sleep(m_start);                  //等待开始时间

    cout << "t=" << m_start / 1000 << ":读者进程" << m_id << "申请读文件." << endl;
    WaitForSingleObject(rmutex, INFINITE);      //等待资源加锁 开始读
    readercount++;                              //读者数++
    if (readercount == 1)                       //第一个读者 等待资源加写锁
        WaitForSingleObject(wmutex, INFINITE);
    ReleaseSemaphore(rmutex, 1, NULL);          //开始阅读 释放readercount
    
    cout << "读者进程" << m_id << "开始读文件." << endl;
    Sleep(m_delay);                             //阅读持续m_delay s
    cout << ":读者进程" << m_id << "完成读文件." << endl;
    
    WaitForSingleObject(rmutex, INFINITE);      //一个人读完 等待资源加读锁
    readercount--;                              //读者数--
    if (readercount == 0)                       //最后一个读者读完 释放readercount
        ReleaseSemaphore(wmutex, 1, NULL);
    ReleaseSemaphore(rmutex, 1, NULL);          //释放读锁 可以后续操作
}

// 读者优先，写者线程
void rp_Writer(void* p)
{
    int m_id;
    DWORD m_start;
    DWORD m_delay;

    //从参数中获得信息
    m_id = ((thread_info*)(p))->id;
    m_start = (DWORD)(((thread_info*)(p))->start * INTE_PER_SEC);
    m_delay = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);

    Sleep(m_start);                 //等待开始时间
    cout << "t=" << m_start / 1000 << ":写者进程" << m_id << "申请写文件." << endl;
    WaitForSingleObject(wmutex, INFINITE);      //等待资源加锁 开始写

    cout << "写者进程" << m_id << "开始写文件." << endl;
    Sleep(m_delay);
    cout << "写者进程" << m_id << "完成写文件." << endl;

    ReleaseSemaphore(wmutex, 1, NULL);          //释放写锁 可以后续操作
}

//读者优先
void ReaderPriority(char* file)
{
    DWORD n_thread = 0;         //线程数目
    DWORD thread_ID;            //线程ID
    DWORD wait_for_all;         //等待所有线程结束

    //创建信号量
    wmutex = CreateSemaphore(NULL, 1, 1, NULL);
    rmutex = CreateSemaphore(NULL, 1, 1, NULL);

    HANDLE h_Thread[MAX_THREAD_NUM];            //各线程的handle
    thread_info thread_info[MAX_THREAD_NUM];
    
    //初始化
    int id = 0;
    readercount = 0;   

    //从文件读信息
    ifstream m_File;
    m_File.open(file); 
    while (m_File)
    {
        m_File >> thread_info[n_thread].id;
        m_File >> thread_info[n_thread].status;
        m_File >> thread_info[n_thread].start;
        m_File >> thread_info[n_thread++].delay;
        m_File.get();
    }   

    cout << "读者优先:" << endl;
    for (int i = 0; i < (int)(n_thread); i++)
    {
        //创建读者进程
        if (thread_info[i].status == READER || thread_info[i].status == 'r')
            
            h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(rp_Reader), &thread_info[i], 0, &thread_ID);
        //创建写者线程
        else
            h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(rp_Writer), &thread_info[i], 0, &thread_ID);
    }

    //等待子线程结束
    //关闭句柄
    wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
    cout << "操作完成" << endl;
    for (int i = 0; i < (int)(n_thread); i++)
        CloseHandle(h_Thread[i]);
    CloseHandle(wmutex);
    CloseHandle(rmutex);
}



/*****************/
//写者优先 读者线程
void wp_Reader(void* p) 
{
    int m_id;
    DWORD m_start;
    DWORD m_delay;

    //从参数中获得信息
    m_id = ((thread_info*)(p))->id;
    m_start = (DWORD)(((thread_info*)(p))->start * INTE_PER_SEC);
    m_delay = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);

    Sleep(m_start);                 //等待开始时间

    cout << "t=" << m_start / 1000 << ":读者进程" << m_id << "申请读文件." << endl;
    WaitForSingleObject(mutex, -1);         //判断写者队列是否有进程
    WaitForSingleObject(rmutex, -1);        //让读者进程先后访问readercount
    WaitForSingleObject(mutex2, -1);        //让读者进程先后访问readercount

    readercount++;
    if (readercount == 1)                   //第一个读者 等待资源加写锁
        WaitForSingleObject(wmutex, -1);
    ReleaseSemaphore(mutex2, 1, NULL);       //释放互斥信号量mutex
    ReleaseSemaphore(rmutex, 1, NULL);      //开始阅读 释放readercount  
    ReleaseSemaphore(mutex, 1, NULL);       //释放互斥信号量mutex

    cout << "读者进程" << m_id << "开始读文件." << endl;
    Sleep(m_delay);
    cout << "读者进程" << m_id << "完成读文件." << endl;

    WaitForSingleObject(mutex2, -1);        //让读者进程先后访问readercount
    readercount--;                          //读者数--
    if (readercount == 0)                   //最后一个读者读完 释放读锁
        ReleaseSemaphore(wmutex, 1, NULL);
    ReleaseSemaphore(mutex2, 1, NULL);      //释放readercount 可以后续操作
}

//写者优先 写者线程
void wp_Writer(void* p) {
    int m_id;
    DWORD m_start;
    DWORD m_delay;

    //从参数中获得信息
    m_id = ((thread_info*)(p))->id;
    m_start = (DWORD)(((thread_info*)(p))->start * INTE_PER_SEC);
    m_delay = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
    
    Sleep(m_start);                         //等待开始时间 

    cout << "t=" << m_start / 1000 << ":写者进程" << m_id << "申请写文件." << endl;

    WaitForSingleObject(mutex3, -1);        //让写者进程先后访问writercount
    writercount++;
    if (writercount == 1)
        WaitForSingleObject(rmutex, -1);     //第一个写者 等待资源加读锁
    ReleaseSemaphore(mutex3, 1, NULL);      //开始阅读 释放writercount

    WaitForSingleObject(wmutex, -1);
    cout << "写者进程" << m_id << "开始写文件." << endl;
    Sleep(m_delay);
    cout << "写者进程" << m_id << "完成写文件." << endl;
    ReleaseSemaphore(wmutex, 1, NULL);

    WaitForSingleObject(mutex3, -1);        //让写者进程先后访问writercount
    writercount--;
    if (writercount == 0)                   //最后一个读者读完 释放写锁
        ReleaseSemaphore(rmutex, 1, NULL);
    ReleaseSemaphore(mutex3, 1, NULL);      //释放writercount 可以后续操作
}

//写者优先
void WriterPriority(char* file) {
    DWORD n_thread = 1;           //线程数目
    DWORD thread_ID;            //线程ID
    DWORD wait_for_all;         //等待所有线程结束

    //创建信号量
    mutex = CreateSemaphore(NULL, 1, 1, NULL);//书籍互斥访问信号量，初值为1,最大值为1
    mutex2 = CreateSemaphore(NULL, 1, 1, NULL);//读者对count修改互斥信号量，初值为1,最大为1
    mutex3 = CreateSemaphore(NULL, 1, 1, NULL);//书籍互斥访问信号量，初值为1,最大值为1
    wmutex = CreateSemaphore(NULL, 1, 1, NULL);//
    rmutex = CreateSemaphore(NULL, 1, 1, NULL);//

    HANDLE h_Thread[MAX_THREAD_NUM];//线程句柄,线程对象的数组
    thread_info thread_info[MAX_THREAD_NUM];

    int id = 0;
    readercount = 0;               //初始化readcount
    writercount = 0;
    ifstream m_File;
    m_File.open(file);
    cout << "写者优先:" << endl;
    while (m_File)
    {
        //读入每一个读者,写者的信息
        m_File >> thread_info[n_thread].id;
        m_File >> thread_info[n_thread].status;
        m_File >> thread_info[n_thread].start;
        m_File >> thread_info[n_thread++].delay;
        m_File.get();
    }
    for (int i = 0; i < (int)(n_thread); i++)
    {
        if (thread_info[i].status == READER || thread_info[i].status == 'r')
        {
            //创建读者进程
            h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(wp_Reader), &thread_info[i], 0, &thread_ID);
        }
        else
        {
            //创建写线程
            h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(wp_Writer), &thread_info[i], 0, &thread_ID);
        }
    }
    //等待子线程结束

    //关闭句柄
    wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
    cout << endl;
    cout << "所有读者写者已经完成操作！！" << endl;
    for (int i = 0; i < (int)(n_thread); i++)
        CloseHandle(h_Thread[i]);
    CloseHandle(mutex);
    CloseHandle(mutex2);
    CloseHandle(rmutex);
    CloseHandle(wmutex);
}

void rl_Reader(void* p)
{
    int m_id;
    DWORD m_start;
    DWORD m_delay;

    //从参数中获得信息
    m_id = ((thread_info*)(p))->id;
    m_start = (DWORD)(((thread_info*)(p))->start * INTE_PER_SEC);
    m_delay = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);

    Sleep(m_start);                  //等待开始时间

    cout << "t=" << m_start / 1000 << ":读者进程" << m_id << "申请读文件." << endl;
    WaitForSingleObject(rmutex, INFINITE);      //等待资源加锁 开始读
    WaitForSingleObject(mutex, INFINITE);       //读者数量锁
    readercount++;                              //读者数++
    if (readercount == 1)                       //第一个读者 等待资源加写锁
        WaitForSingleObject(wmutex, INFINITE);
    ReleaseSemaphore(rmutex, 1, NULL);          //开始阅读 释放读锁

    cout << "读者进程" << m_id << "开始读文件." << endl;
    Sleep(m_delay);                             //阅读持续m_delay s
    cout << ":读者进程" << m_id << "完成读文件." << endl;
    ReleaseSemaphore(mutex, 1, NULL);           //开始阅读 释放读锁

    WaitForSingleObject(rmutex, INFINITE);      //一个人读完 等待资源加读锁
    readercount--;                              //读者数--
    if (readercount == 0)                       //最后一个读者读完 释放写锁
        ReleaseSemaphore(wmutex, 1, NULL);
    ReleaseSemaphore(rmutex, 1, NULL);          //释放读锁 可以后续操作
}

// 读者优先，写者线程
void rl_Writer(void* p)
{
    int m_id;
    DWORD m_start;
    DWORD m_delay;

    //从参数中获得信息
    m_id = ((thread_info*)(p))->id;
    m_start = (DWORD)(((thread_info*)(p))->start * INTE_PER_SEC);
    m_delay = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);

    Sleep(m_start);                 //等待开始时间
    cout << "t=" << m_start / 1000 << ":写者进程" << m_id << "申请写文件." << endl;
    WaitForSingleObject(wmutex, INFINITE);      //等待资源加锁 开始写

    cout << "写者进程" << m_id << "开始写文件." << endl;
    Sleep(m_delay);
    cout << "写者进程" << m_id << "完成写文件." << endl;

    ReleaseSemaphore(wmutex, 1, NULL);          //释放写锁 可以后续操作
}

//读者优先
void ReaderLimit(char* file)
{
    DWORD n_thread = 1;         //线程数目
    DWORD thread_ID;            //线程ID
    DWORD wait_for_all;         //等待所有线程结束

    //创建信号量
    wmutex = CreateSemaphore(NULL, 1, 1, NULL);
    rmutex = CreateSemaphore(NULL, 1, 1, NULL);
    mutex = CreateSemaphore(NULL, 2, 2, NULL);


    HANDLE h_Thread[MAX_THREAD_NUM];            //各线程的handle
    thread_info thread_info[MAX_THREAD_NUM];

    //初始化
    int id = 0;
    readercount = 0;

    //从文件读信息
    ifstream m_File;
    m_File.open(file);
    while (m_File)
    {
        m_File >> thread_info[n_thread].id;
        m_File >> thread_info[n_thread].status;
        m_File >> thread_info[n_thread].start;
        m_File >> thread_info[n_thread++].delay;
        m_File.get();
    }

    cout << "读者优先:" << endl;
    for (int i = 0; i < (int)(n_thread); i++)
    {
        //创建读者进程
        if (thread_info[i].status == READER || thread_info[i].status == 'r')

            h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(rl_Reader), &thread_info[i], 0, &thread_ID);
        //创建写者线程
        else
            h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(rl_Writer), &thread_info[i], 0, &thread_ID);
    }

    //等待子线程结束
    //关闭句柄
    wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
    cout << "操作完成" << endl;
    for (int i = 0; i < (int)(n_thread); i++)
        CloseHandle(h_Thread[i]);
    CloseHandle(wmutex);
    CloseHandle(rmutex);
}





//主函数
int main()
{
    char choice;
    cout << "    欢迎进入读者写者模拟程序    " << endl;
    while (true)
    {
        //打印提示信息
        cout << "     请输入你的选择       " << endl;
        cout << "     1、读者优先" << endl;
        cout << "     2、写者优先" << endl;
        cout << "     3、读者限定" << endl;
        cout << "     4、退出程序" << endl;
        cout << endl;
        //如果输入信息不正确，继续输入
        do {
            choice = (char)_getch();
        } while (choice != '1' && choice != '2' && choice != '3' && choice != '4');

        system("cls");
        //选择1，读者优先
        if (choice == '1')
            //ReaderPriority("thread.txt");
            ReaderPriority(const_cast<char*>("thread.txt"));
        //选择2，写者优先
        else if (choice == '2')
            WriterPriority(const_cast<char*>("thread.txt"));
        //选择3，退出
        else if(choice=='3')
            ReaderLimit(const_cast<char*>("thread.txt"));
        else 
            return 0;
        //结束
        printf("\nPress Any Key to Coutinue");
        _getch();
        system("cls");
    }
    return 0;
}
