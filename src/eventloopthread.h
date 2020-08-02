// evnetloopthread代表这一个线程实例




#ifndef ROOT_SRC_EVENTLOOPTHREAD_H
#define ROOT_SRC_EVENTLOOPTHREAD_H

#include "eventloop.h"

#include <condition_variable>
#include <mutex>
#include <thread>


namespace summer
{
    class eventloopthread
    {
    public:
        eventloopthread();
        ~eventloopthread();
        eventloop* startloop();

    private:
        //线程主函数
        void threadFunc();

        //持有一个eventloop对象
        eventloop* loop_;

        bool exiting_;

        //线程对象
        std::thread thread_;

        // 控制线程开始
        bool start_;
        
        std::mutex mutex_;
        std::condition_variable cond_;
    };
} // end summer




#endif