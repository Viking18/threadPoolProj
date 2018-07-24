//
// Created by 经纬 on 2018/7/24.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "myThreadPool.h"

#define DEBUG(i,j) printf("THREAD %ld: %s\n",i,j)

MyThreadPool::MyThreadPool(int max, int core, time_t * timespec1, MyWorkQueue *queue)
{
    this->maxThreadNum = max;
    this->coreThreadNum = core;
    this->workQueue = queue;
    this->currentThreadNum = 0;
    this->timeWait = timespec1;
}

int MyThreadPool::init()
{
    /*
     * 1. 根据max,core设置pool对象的属性值（高级设计，应该接收一个attr结构体，方便扩展）
     * 2. 初始化tidArray, 存储所属的工作线程的tid， 为-1则为对应位置为存储
     * 3. 创建核心数目的工作线程，将workQueue的地址传给他们
     */
    int iRet = 0;
    this->currentThreadNum = 0;

    //init mutex_t and cond_t
    iRet = pthread_cond_init(&cond, nullptr);
    iRet |= pthread_mutex_init(&lock, nullptr);
    if(iRet!=0)
    {
        printf("ERROR: init pthread_cond and pthread_mutex failed!\n");
        return iRet;
    }

//    //init tidArray
//    if( (this->tidArray = (pthread_t *) malloc(sizeof(pthread_t) * this->maxThreadNum))== nullptr)
//    {
//        printf("ERROR: malloc tidArray error!\n");
//        iRet = -1;
//        return iRet;
//    }
//    for(int i=0;i < this->maxThreadNum; i++)
//    {
//        tidArray[i] = (pthread_t) -1;
//    }
//
//
//    //create core threads
//    while(this->currentThreadNum < this->coreThreadNum)
//    {
//        pthread_t tid;
//        if( (iRet = pthread_create(&tid, nullptr, _workThread ,this))!=0)
//        {
//            printf("ERROR: create thread error!\n");
//            //if failed,cancel thread
//            for(int i=0;i<this->maxThreadNum;i++)
//            {
//                if( this->tidArray[i] != (pthread_t) -1)
//                    pthread_cancel(this->tidArray[i]);
//            }
//            free(this->tidArray);
//            free(this->workQueue);
//            return iRet;
//        }
//
//        tidArray[this->currentThreadNum] = tid;
//        ++ this->currentThreadNum;
//    }

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
            DEBUG((long)pthread_self()%177, "_workThread: waiting for work...");
            timespec tmp;
            tmp.tv_sec = time(nullptr) + *pool->timeWait;
            pthread_cond_timedwait(&pool->cond, &pool->lock, &tmp);
            if(time(nullptr) >= tmp.tv_sec) //超时，退出线程
            {
                DEBUG((long)pthread_self()%177, "time exceed. return!");
                -- pool->currentThreadNum;
                pthread_mutex_unlock( &pool->lock);
                return nullptr;
            }
        }

        node = pool->workQueue->getWork();
        pthread_mutex_unlock( &pool->lock);

        if(node == nullptr)
        {
            DEBUG((long)pthread_self()%177, "_workThread: get no work");
            continue;
        }
        else {
//            DEBUG((long)pthread_self()%177, "_workThread: start processing...");
            node->call(node->para);
            //TODO 对于返回结果的存储或处理
            DEBUG((long)pthread_self()%177, "_workThread: job done.");
        }
    }
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
        DEBUG((long)pthread_self()%177, "submit: malloc new MyWorkQueueNode failed");
        return -2;
    }

    newNode->call = call;
    newNode->para = para;
    newNode->next = nullptr;

    //线程数量小于核心线程数量，直接创建线程
    pthread_mutex_lock(&this->lock);
    if(this->currentThreadNum < this->coreThreadNum)
    {
        pthread_t tid;
        if( (iRet = pthread_create( &tid, nullptr, _workThread, this)) != 0)
        {
            DEBUG((long)pthread_self()%177, "pthread_create failed");
        }
        else
        {
            printf("raise new thread %ld\n",(long)tid%177);
            ++ this->currentThreadNum;
        }
    }

    //如果缓冲队列不满，直接放入队列
    if(!this->workQueue->isFull())
    {
        this->workQueue->putWork(newNode);
    }
    else
    {
        //队列已满，检测是否达到最大线程数
        if (this->currentThreadNum < this->maxThreadNum)
        {
            pthread_t tid;
            if ((iRet = pthread_create(&tid, nullptr, _workThread, this)) != 0) {
                DEBUG((long)pthread_self()%177, "pthread_create failed");
            } else {
                printf("raise new thread %ld\n", (long)tid%177);
                ++this->currentThreadNum;
            }
            this->workQueue->putWork(newNode);
        }
        else
        {
            //队列已满，线程最大，返回失败
            DEBUG((long)pthread_self()%177, "REJECTED! thread num max, work queue full.");
            pthread_mutex_unlock( &this->lock);
            return -1;
        }
    }
    pthread_mutex_unlock( &this->lock);
    pthread_cond_signal( &this->cond);
    return 0;
}

MyWorkQueue::MyWorkQueue(int max)
{
    this->maxWorkNum = max;
    MyWorkQueueNode * node = new MyWorkQueueNode();

    this->head = node;
    this->tail = node;
    this->currentWorkNum = 0;

}

int MyWorkQueue::putWork(MyWorkQueueNode *node)
{
    /*
     * return 0 success; -1 refused;
     */

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

bool MyWorkQueue::isFull()
{
    return (this->currentWorkNum > this->maxWorkNum);
}

MyWorkQueueNode * MyWorkQueue::getWork()
{
    if (this->isEmpty())
        return nullptr;

    MyWorkQueueNode * res;
    res = this->head->next;
    if(res == nullptr)
    {
        DEBUG((long)pthread_self() % 177, "ERROR: workQueue get failed. FATAL ERROR!");
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


