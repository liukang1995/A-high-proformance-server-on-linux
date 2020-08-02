

#ifndef ROOT_SRC_EVENTLOOPTHREADPOOL_H
#define ROOT_SRC_EVENTLOOPTHREADPOOL_H

#include "eventloop.h"
#include "eventloopthread.h"

#include <memory>
#include <vector>

namespace summer
{
    class eventloopthreadpool
    {
    public:
        eventloopthreadpool( eventloop* baseloop, int numthreads );

        ~eventloopthreadpool();

        void start();

        eventloop* getnextloop();

    private:
        bool started_;

        eventloop* baseloop_;

        // 线程池线程数量
        int numthreads_;
        
        // 用于标识下一个线程的索引
        int next_;

        // 线程池
        std::vector<std::shared_ptr<eventloopthread>> threads_;

        std::vector<eventloop*> loops_;
    };
} // end summer




#endif