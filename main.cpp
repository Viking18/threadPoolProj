#include <iostream>
#include <vector>
#include <algorithm>
#include "myThreadPool.h"
using namespace std;

#define maxThread 4
#define coreThread 4
#define maxWork 10



struct addPare
{
    int a,b;
    int sum;
};

void add(void *);

int main(int argc, char *argv[]) {
    int iRet;

    if(argc != 2)
    {
        printf("usage  threadPoolProj <workNum>");
        return -1;
    }

    MyThreadPool * pool = (MyThreadPool *) malloc(sizeof(MyThreadPool));
    if(pool == nullptr)
    {
        printf("malloc MyThreadPool failed;");
        return -1;
    }

    if( (iRet = pool->init(maxThread, coreThread, maxWork))!=0)
    {
        printf("pool->init failed;\n");
        return iRet;
    }


    int n = atoi(argv[1]);
    addPare pare[n];
    for(int i=0;i<n;i++)
    {
        pare[i].a = i;
        pare[i].b = 2 * i;
        pare[i].sum = -1;
        pool->submit(add,&pare[i]);
    }

    for(int i=0;i<1000000;i++);
    for(int i=0;i<n;i++)
    {
        printf("i:%d, job result: %d\n", i, pare[i].sum);
    }

    return 0;

}

void add(void * pare)
{
    addPare * addPare1 = (addPare *) pare;
    addPare1->sum = addPare1->a + addPare1->b;
}