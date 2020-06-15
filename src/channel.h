// channel负责一个fd的IO事件分发


#ifndef ROOT_SRC_CHANNEL_H
#define ROOT_SRC_CHANNEL_H

#include "sys/epoll.h"

#include "cstdint"
#include <functional>

namespace summer
{
    class eventloop; //前向声明

    class channel
    {
    private:
        typedef std::function<void()> callback;
        // 持有的文件描述符
        int fd_;
        // 指向负责fd_线程的loop
        eventloop* loop_;
        // 关心的事件
        uint32_t events;
        // 由epoll设定，发生的事件
        uint32_t revents;


        // + 记录上一次关心的事件，在更新的时候用
        uint32_t lastevents;

        callback readcallback_;
        callback writecallback_;
        callback errorcallback_;
        callback closecallback_;

    public:
        channel( eventloop* , int );
        ~channel();

        // 根据感知的事件调用相应的处理函数
        void handleEvents();
        void setreadcallback( callback c) { readcallback_ = std::move(c); }
        void setwritecallback( callback c) { writecallback_ = std::move(c); }
        void seterrorcallback( callback c) { errorcallback_ = std::move(c); }
        void setclosecallback( callback c) { closecallback_ = std::move(c); }

        int getFd(){ return fd_; }
        uint32_t getEvents() { return events; }
        void setrEvents( uint32_t ev ){ revents = ev; }

        void setEvents( uint32_t ev ){ events = ev; }

        bool compare_update_lastevents();

    };
















} // end summer
#endif