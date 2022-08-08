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
    thread *t = new thread(&ThreadPool::manage, this);//启用一个管理线程
}

ThreadPool::~ThreadPool()
{
    exit();
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
    //cout << "add task success" << endl;
    threadCon.notify_one();
}

void ThreadPool::work()
{  
    while(1)
    {
        taskQueueMutex.lock();
        if (!taskQueue.empty())//如果task队列有task就取一个执行
        {
            Task topTask(*(taskQueue.front()));
            taskQueue.pop();
            taskQueueMutex.unlock();
            topTask();
            cout << "thread id: " << this_thread::get_id() << "work" << endl;
        }
        else//没有任务就阻塞当前线程
        {
            //cout << "thread id: "<< this_thread::get_id() <<" block, task queue is empty" << endl;
            taskQueueMutex.unlock();//先将任务队列解锁
            unique_lock<mutex> threadLock(threadMutex);
            threadCon.wait(threadLock);

            reduceNumMutex.lock();//如果线程减少标志大于0，就让当前线程自动结束
            if(reduceNum > 0)
            {
                --reduceNum;
                reduceNumMutex.unlock();
                threadVectorMutex.lock();
                for (auto it = threadVector.begin(); it != threadVector.end(); ++it)
                {
                    if((*it)->get_id() == this_thread::get_id())
                    {
                        threadVector.erase(it);
                        threadVectorMutex.unlock();
                        break;
                    }
                }
                cout << "reduce one thread" << endl;
                threadVectorMutex.unlock();
                break;
            } 
            reduceNumMutex.unlock();
        }
    }
}

void ThreadPool::manage()
{
    cout << "start manage" << endl;
    while(1)
    {
        sleep(4); //每4秒检测一次
        threadVectorMutex.lock();
        int threadNum = threadVector.size();
        threadVectorMutex.unlock();

        taskQueueMutex.lock();
        int taskNum = taskQueue.size();
        taskQueueMutex.unlock();

        //任务数量太多，增加线程
        if(taskNum > 2 * threadNum && threadNum < maxThreadNum)
        {
            for (int i = 0; i < increaseNum; ++i)
            {
                thread *t = new thread(&ThreadPool::work, this);
                //t->detach();
                threadVectorMutex.lock();
                threadVector.emplace_back(t);
                threadVectorMutex.unlock();
                cout << "add one thread" << endl;
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

        taskQueueMutex.lock();
        if(taskQueue.empty())
        {
            taskQueueMutex.unlock();
            exit();
            break;
        }
        taskQueueMutex.unlock();
    }
    cout << "stop manage" << endl;
}

 void ThreadPool::exit()
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
    if(!threadVector.empty())
    {
        for (auto t : threadVector)
        {
            if(t->joinable())
            {
                t->join();
            }
            delete t;
        }
        threadVector.clear();
    }
    threadVectorMutex.unlock();
    //cout << "pool exit" << endl;
 }