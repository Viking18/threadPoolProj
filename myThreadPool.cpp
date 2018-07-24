//
// Created by 经纬 on 2018/7/24.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "myThreadPool.h"

#define DEBUG(i,j) printf("THREAD %d: %s\n",i,j)


int MyThreadPool::init(int max,int core,int maxWork)
{
    /*
     * 1. 根据max,core设置pool对象的属性值（高级设计，应该接收一个attr结构体，方便扩展）
     * 2. 初始化tidArray, 存储所属的工作线程的tid， 为-1则为对应位置为存储
     * 3. 创建核心数目的工作线程，将workQueue的地址传给他们
     */
    int iRet = 0;
    this->maxThreadNum = max;
    this->coreThreadNum = core;
    this->currentThreadNum = 0;

    //init mutex_t and cond_t
    iRet = pthread_cond_init(&cond, nullptr);
    iRet |= pthread_mutex_init(&lock, nullptr);
    if(iRet!=0)
    {
        printf("ERROR: init pthread_cond and pthread_mutex failed!\n");
        return iRet;
    }

    //init tidArray
    if( (this->tidArray = (pthread_t *) malloc(sizeof(pthread_t) * this->maxThreadNum))== nullptr)
    {
        printf("ERROR: malloc tidArray error!");
        iRet = -1;
        return iRet;
    }
    for(int i=0;i < this->maxThreadNum; i++)
    {
        tidArray[i] = (pthread_t) -1;
    }

    //init workQueue
    this->workQueue = (MyWorkQueue *) malloc(sizeof(MyWorkQueue));
    if( (iRet = this->workQueue->init(maxWork))!=0)
    {
        printf("ERROR: init workQueue error;iRet:%d, maxWork:%d\n", iRet, maxWork);
        free(this->tidArray);
        return iRet;
    }

    //create core threads
    while(this->currentThreadNum < this->coreThreadNum)
    {
        pthread_t tid;
        if( (iRet = pthread_create(&tid, nullptr, _workThread ,this))!=0)
        {
            printf("ERROR: create thread error!");
            //if failed,cancel thread
            for(int i=0;i<this->maxThreadNum;i++)
            {
                if( this->tidArray[i] != (pthread_t) -1)
                    pthread_cancel(this->tidArray[i]);
            }
            free(this->tidArray);
            free(this->workQueue);
            return iRet;
        }

        tidArray[this->currentThreadNum] = tid;
        ++ this->currentThreadNum;
    }

    return 0;
}

void * MyThreadPool::_workThread(void *args)
{
    /*
     * 1. 阻塞于从队列提取任务， 等待控制线程发信号唤醒
     * 2. 被唤醒后，应当取得，才能从队列中获取任务
     * 3. 得到任务后，应该释放锁
    */
    MyWorkQueueNode * node;
    MyThreadPool * pool =(MyThreadPool *) args;

    for(;;)
    {
        pthread_mutex_lock( &pool->lock);
        while(pool->workQueue->isEmpty())
        {
            DEBUG(pthread_self(), "_workThread: waiting for work...");
            pthread_cond_wait(&pool->cond, &pool->lock);
        }

        node = pool->workQueue->getWork();
        pthread_mutex_unlock( &pool->lock);

        if(node == nullptr)
        {
            DEBUG(pthread_self(), "_workThread: get no work");
            continue;
        }
        else {
            DEBUG(pthread_self(), "_workThread: start processing...");
            node->call(node->para);
            //TODO 对于返回结果的存储或处理
            DEBUG(pthread_self(), "_workThread: job done.");
        }
    }
}

int MyWorkQueue::init(int max)
{
    MyWorkQueueNode * newNode =(MyWorkQueueNode *) malloc(sizeof(MyWorkQueueNode));
    if(newNode == nullptr)
    {
        DEBUG(pthread_self(), "malloc MyWorkQueueNode failed!");
        return -2;
    }

    newNode->next = nullptr;
    this->head = newNode;
    this->tail = newNode;
    this->maxWorkNum = max;
    this->currentWorkNum = 0;

    return 0;
}

int MyThreadPool::submit(void (*call)(void *), void *para)
{
    /*
     * 1. 获取mutex，
     * 2. 检查总任务数量，决定是否新建线程/拒绝服务/添加任务
     * 3.
     * @return
     */
    MyWorkQueueNode * newNode;
    int iRet;
    if ( (newNode =(MyWorkQueueNode*) malloc(sizeof(MyWorkQueueNode)))== nullptr)
    {
        DEBUG(pthread_self(), "submit: malloc new MyWorkQueueNode failed");
        return -2;
    }

    newNode->call = call;
    newNode->para = para;
    newNode->next = nullptr;

    pthread_mutex_lock( &this->lock);
    iRet = this->workQueue->putWork(newNode);
    pthread_mutex_unlock( &this->lock);
    pthread_cond_signal( &cond);
    if( iRet == 0)
    {
        DEBUG(pthread_self(), "submit: put work success!");
    }
    else if(iRet == -1)
    {
        DEBUG(pthread_self(), "submit: too much work,refused!");
    }

    return iRet;
}


int MyWorkQueue::putWork(MyWorkQueueNode *node)
{
    /*
     * return 0 success; -1 refused;
     */
    if (this->currentWorkNum >= this->maxWorkNum)
    {
        DEBUG(pthread_self(), "putWork: workQueue full. Refused!");
        return -1;
    }

    this->tail->next = node;
    node->next = nullptr;
    this->tail = node;

    ++currentWorkNum;
    return 0;
}

bool MyWorkQueue::isEmpty()
{
    return this->head==this->tail;
}

MyWorkQueueNode * MyWorkQueue::getWork()
{
    if (this->isEmpty())
        return nullptr;

    MyWorkQueueNode * res;
    res = this->head->next;
    if(res == nullptr)
    {
        DEBUG(pthread_self(), "ERROR: workQueue get failed. FATAL ERROR!");
        return nullptr;
    }

    this->head->next = res->next;
    if(res->next == nullptr ) //last node
    {
        this->tail = this->head;
    }

    -- this->currentWorkNum;

    return res;
}
