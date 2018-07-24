//
// Created by 经纬 on 2018/7/24.
//
#include <pthread.h>
#ifndef THREADPOOLPROJ_MYTHREADPOOL_H
#define THREADPOOLPROJ_MYTHREADPOOL_H

struct MyWorkQueueNode
{
    MyWorkQueueNode* next;
//    MyWorkQueueNode* prev;

    // 可以添加优先级等
    void  (*call)(void *);
    void *  para;
};


class MyWorkQueue
{
private:
    int     maxWorkNum;
    int     currentWorkNum;
    MyWorkQueueNode* head;
    MyWorkQueueNode* tail;

public:
    int     init(int max);
    int     putWork(MyWorkQueueNode * node);
//    int     removeWork();
    bool    isEmpty();
    MyWorkQueueNode*     getWork();
};

class MyThreadPool
{
private:
    int     maxThreadNum,coreThreadNum;
    int     currentThreadNum;
    MyWorkQueue *   workQueue;
    pthread_t *     tidArray;

    pthread_cond_t  cond;
    pthread_mutex_t lock;
public:
    static  void *  _workThread(void *arg);
    int     init(int max, int core, int maxWork);
    int     submit(void (*call)(void *), void * para);
};

#endif //THREADPOOLPROJ_MYTHREADPOOL_H
