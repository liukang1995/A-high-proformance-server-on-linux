//author：liukang
//thread pool
#pragma once

#include "eventloop.h"
#include "loopthread.h"

#include <memory>
#include <vector>


namespace summer
{
    class threadpool
    {
    public:
        threadpool( int numthreads );
        threadpool(const threadpool&) = delete;
        threadpool& operator=(const threadpool&) = delete;

        ~threadpool() = default;

        void start();
        summer::eventloop* getnextloop();
    
    private:
        bool start_;
        int next_;
        int numthreads_;
    //    eventloop* baseloop_;
        std::vector<eventloop*> loops_;
        std::vector<std::shared_ptr<loopthread>> threads_;
    };
} // namespace summer
