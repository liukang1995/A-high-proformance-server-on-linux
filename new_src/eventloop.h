// author : liukang
// rep a evnet loop 文件事件处理器

#pragma once

#include "channel.h"
#include "epoll.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace summer
{
    class eventloop
    {
    public:
        using func = std::function<void()>;
        eventloop();
        eventloop(const eventloop&) = delete;
        
        ~eventloop(); // unable move

        void quit();
        void loop();

        void runinloop(func&& f); // on current thread proc at once 
        void queueinloop(func&& f); // add to pendingfunctors

        void wakeup();
    //    void shutdown_write( std::shared_ptr<channel> chan);

        std::shared_ptr<epoll> getepoll() { return epoller_; }
    private:
        bool looping_; // in loop func
        std::atomic<bool> quit_; // control loop
        bool eventHanding_; // handing IO  event
        std::thread::id Thread_Id_;
        std::shared_ptr<epoll> epoller_;

        int wakefd_;
        std::shared_ptr<channel> wakeupchannel_;
        void wakefdRead();
    
        std::mutex mx_;
        bool callingfunctors_; // calling pending functors
        std::vector<func> pendfunctors;
        void Procfunctors();
    };
}