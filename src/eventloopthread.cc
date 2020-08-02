#include "eventloopthread.h"

#include <functional>

using namespace std;
using namespace summer;

eventloopthread::eventloopthread()
    :
    loop_(nullptr),
    mutex_(),
    cond_(),
    start_(false),
    exiting_(false),
    thread_( std::bind(threadFunc,this) )
{

}

eventloopthread::~eventloopthread()
{
    exiting_ = true;
    if( loop_ )
    {
        loop_->quit();
        thread_.join();
    }
}

void eventloopthread::threadFunc()
{
    //控制线程执行，若未运行startloop()，则线程一直阻塞在cond_上
    unique_lock<mutex> lck;
    // 闭包传递this可行吗？
    cond_.wait( lck,[this]{ return start_; } );


    eventloop loop;

    {
        // loop_ 是多线程可访问的
        lock_guard<mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    // 真正的线程函数
    loop.loop();

    lock_guard<mutex> lock(mutex_);
    loop_ = nullptr;
}

// 控制线程正式运行，并返回栈上loop对象的地址
eventloop* eventloopthread::startloop()
{
    // 改变start_的状态，若线程阻塞start_上，则恢复运行，若还未阻塞在start_上，运行到时，直接跳过
    {
        lock_guard<mutex> lck(mutex_);
        start_ = true;
        cond_.notify_all();
    }

    // 等待栈上对象evntloop建立完毕
    {
        unique_lock<mutex> lck(mutex_);
        cond_.wait(lck, [this]{ return loop_ != nullptr; });
    }
    return loop_;
}

