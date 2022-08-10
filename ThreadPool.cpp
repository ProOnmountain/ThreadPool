#include "ThreadPool.h"
#include<iostream>
#include<unistd.h>

ThreadPool::ThreadPool(int minNum, int maxNum):
minThreadNum(minNum), maxThreadNum(maxNum), reduceNum(0), increaseNum(2), exitFlag(0)
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
            //cout << "thread id: " << this_thread::get_id() << " work" << endl;
        }
        else//没有任务就阻塞当前线程
        {
            //cout << "thread id: "<< this_thread::get_id() <<" block, task queue is empty" << endl;
            taskQueueMutex.unlock();//先将任务队列解锁
            unique_lock<mutex> threadLock(threadMutex);
            threadCon.wait(threadLock);
            reduceNumMutex.lock(); //如果线程减少标志大于0，就让当前线程自动结束
            //cout << "reduce num: " << reduceNum << endl;
            if (reduceNum > 0)
            {
                --reduceNum;
                reduceNumMutex.unlock();
                threadVectorMutex.lock();
                for (auto it = threadVector.begin(); it != threadVector.end(); ++it)
                {
                    if((*it)->get_id() == this_thread::get_id())
                    {
                        threadVector.erase(it);
                        break;
                    }
                }
                //cout << "reduce one thread, thread size: " << threadVector.size() << endl;
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
        exitFlagMutex.lock();
        if(exitFlag == -1)//exit调用成功，直接退出manage
        {
            exitFlagMutex.unlock();
            break;
        }
        exitFlagMutex.unlock();
        sleep(3); //每4秒检测一次
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
                //cout << "add one thread, thread num: " <<threadVector.size() << endl;
                threadVectorMutex.unlock();

            }
        }

        //任务数量少，减少线程
        if(threadNum > 2 * taskNum && threadNum > minThreadNum)
        {
            exitFlagMutex.lock();
            if(exitFlag != 1)//在没有执行退出程序时才执行下列操作
            {
                exitFlagMutex.unlock();
                reduceNumMutex.lock();
                reduceNum = 2;
                exitFlagMutex.unlock();
                reduceNumMutex.unlock();
                threadCon.notify_all();
            }
        }
        //cout << "manage running" << endl;
    }
    cout << "stop manage" << endl;
}

 void ThreadPool::exit()
 {
     exitFlagMutex.lock();
     if (exitFlag == -1)
     {
         return;
    }
    else
    {
        exitFlag = 1;
    }
    exitFlagMutex.unlock();
    
    //任务没执行完
    while(1)
    {
        taskQueueMutex.lock();
        //cout << "exiting, task size: " << taskQueue.size() << endl;
        if (taskQueue.size() > 0)
        {
            taskQueueMutex.unlock();
        }
        else
        {
            taskQueueMutex.unlock();
            break;
        }
        sleep(1);//间隔1秒检测一次
    }
    
    //把存活线程都结束
    threadVectorMutex.lock();
    if(!threadVector.empty())
    {
        reduceNumMutex.lock();
        reduceNum = threadVector.size();
        threadVectorMutex.unlock();
        reduceNumMutex.unlock();
        
        while(1)
        {
            threadVectorMutex.lock();
            //cout << "exiting, thread size : " << threadVector.size() << endl;
            if(threadVector.size() > 0)
            {
                threadVectorMutex.unlock();
                threadCon.notify_one();
            }
            else
            {
                threadVectorMutex.unlock();
                break;
            }
            sleep(1);
        }
    }
    threadVectorMutex.unlock();
    exitFlagMutex.lock();
    exitFlag = -1;//成功退出
    exitFlagMutex.unlock();
    sleep(1);
    cout << "pool exit" << endl;
 }
