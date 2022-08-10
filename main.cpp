#include "ThreadPool.h"
#include<iostream>
#include <unistd.h>
using namespace std;
void func(void *arg)
{
    int c = *((int *)arg);
    //sleep(1);
    cout << "output: " << c << endl;
    
}

int main()
{
    ThreadPool pool(2, 10);
    
    
    for (int i = 0; i < 20; ++i)
    {
        int *a = new int;
        (*a) = i;
        Func f = func;
        Task *t = new Task(&f, (void*)a);
        pool.addTask(t);
    }
    pool.exit();
    return 0;
}