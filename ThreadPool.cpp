#include "ThreadPool.h"
#include<iostream>
#include<unistd.h>

ThreadPool::ThreadPool(int minNum, int maxNum):
minThreadNum(minNum), maxThreadNum(maxNum), reduceNum(0), increaseNum(2)
{
    for (int i = 0; i < minThreadNum; ++i)
    {
        thread *t = new thread(&ThreadPool::work, this);
        threadVectorMutex.lock();
        threadVector.emplace_back(t);
        threadVectorMutex.unlock();
    }
}

ThreadPool::~ThreadPool()
{
    taskQueueMutex.lock();
    while (taskQueue.size() > 0)
    {
        taskQueueMutex.unlock();
        threadCon.notify_all();
        sleep(2);
    }
    taskQueueMutex.unlock();

    threadVectorMutex.lock();
    for (auto t : threadVector)
    {
        if(t->joinable())
        {
            t->join();
        }
    }
    threadVectorMutex.unlock();

    for(auto t : threadVector)
    {
        delete t;
    }
}

void ThreadPool::addTask(Task *t)
{
    if(t == nullptr)
    {
        cout << "null task" << endl;
        return;
    }

    taskQueueMutex.lock();
    taskQueue.emplace(t);
    taskQueueMutex.unlock();
    cout << "add task success" << endl;
    threadCon.notify_one();
}

void ThreadPool::work()
{
    taskQueueMutex.lock();
    while(1)
    {
        if (!taskQueue.empty())//如果task队列有task就取一个执行
        {
            Task topTask(*(taskQueue.front()));
            taskQueue.pop();
            taskQueueMutex.unlock();
            topTask();
        }
        else//没有任务就阻塞当前线程
        {
            cout << "thread id: "<< this_thread::get_id() <<" block, task queue is empty" << endl;
            taskQueueMutex.unlock();//先将任务队列解锁
            unique_lock<mutex> threadLock(threadMutex);
            threadCon.wait(threadLock);

            reduceNumMutex.lock();
            if(reduceNum > 0)
            {
                --reduceNum;
                reduceNumMutex.unlock();
                break;
            } 
            reduceNumMutex.unlock();
        }
    }
}

void ThreadPool::manage()
{
    while(1)
    {
        sleep(2);//每2秒检测一次
        threadVectorMutex.lock();
        int threadNum = threadVector.size();
        threadMutex.unlock();

        taskQueueMutex.lock();
        int taskNum = taskQueue.size();
        taskQueueMutex.unlock();

        //任务数量太多，增加线程
        if(taskNum > 2 * threadNum && threadNum < maxThreadNum)
        {
            for (int i = 0; i < increaseNum; ++i)
            {
                thread *t = new thread(&ThreadPool::work, this);
                t->detach();
                threadVectorMutex.lock();
                threadVector.emplace_back(t);
                threadVectorMutex.unlock();
            }
        }

        //任务数量少，减少线程
        if(threadNum > 2 * taskNum)
        {
            reduceNumMutex.lock();
            reduceNum = 2;
            reduceNumMutex.unlock();
            threadCon.notify_all();
        }
    }
}
