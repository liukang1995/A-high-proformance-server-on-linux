// author： liukang
// loopthread: a independent loop thread
#pragma once

#include "eventloop.h"
#include <mutex>
#include <future>
#include <thread>

namespace summer
{
    class loopthread
    {
    public:
        loopthread();
        loopthread(const loopthread&) = delete;
        loopthread& operator=(const loopthread&) = delete;
        loopthread(loopthread&&) = default;
        loopthread& operator=(loopthread&&) = default;
        ~loopthread();

        eventloop* start(); // called by other thread
    private:
        void threadFunc(); // thread func

        eventloop* loop_; // thread run it will be set
        bool exit_;
        bool start_;
        std::promise<void> pm_; // control the loop start
        std::promise<void> eventloopcreate_; 
        std::thread loopthread_; // must put it underside
    };
}// end summer