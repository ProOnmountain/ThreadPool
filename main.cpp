#include "ThreadPool.h"
#include<iostream>
#include <unistd.h>
using namespace std;
void func(void *arg)
{
    int c = *((int *)arg);
    cout << "output: " << c << endl;
}

int main()
{
    ThreadPool pool(2, 10);
    int *a = new int;
    *a = 0;
    for (int i = 0; i < 4; ++i)
    {
        (*a) += i;
        Func f = func;
        Task *t = new Task(&f, (void*)a);
        pool.addTask(t);
        sleep(1);
    }
    //pool.manage();
    return 0;
}