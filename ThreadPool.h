/*
C++写的一个简单的线程池
*/
#include<queue>
#include<vector>
#include<thread>
#include<mutex>
#include<condition_variable>

using namespace std;

typedef void (*Func)(void *);//定义任务类型，void(void*)的函数
class Task
{
public:
    Task()
    {
        fun = nullptr;
        arg = nullptr;
    }

    Task(Func *f, void *a)
    {
        fun = f;
        arg = a;
    }

    Task(Task &t)
    {
        fun = t.fun;
        arg = t.arg;
    }

    ~Task()
    {
        // if(fun != nullptr)
        // {
        //     delete fun;
        // }
        // if(arg != nullptr)
        // {
        //     delete arg;
        // }
        fun = nullptr;
        arg = nullptr;
    }

    void operator() ()
    {
        if(fun != nullptr)
        {
            (*fun)(arg);
        }
    }

public:
    Func *fun;
    void *arg;
};

class ThreadPool
{
public:
    ThreadPool(int minNum, int maxNum);
    ~ThreadPool();
    void addTask(Task *t);//向线程池添加任务
    void exit();

private:
    void manage();//管理线程池，任务数量多就增加线程，少就减少线程
    void work();
    
    

private:
    int maxThreadNum;
    int minThreadNum;
    queue<Task*> taskQueue;
    vector<thread*> threadVector;
    int reduceNum;
    int increaseNum;
    int exitFlag;

    mutex threadVectorMutex;
    mutex reduceNumMutex;
    mutex taskQueueMutex;
    mutex threadMutex;
    mutex exitFlagMutex;
    condition_variable threadCon;
};