// author: liukang

#pragma once

#include "channel.h"
#include "timer.h"

#include "sys/epoll.h"
#include "unistd.h"

#include <map>
#include <memory>
#include <vector>

namespace summer
{
    class EPOLL_ADD { };
    class EPOLL_DELETE { };
    class EPOLL_MOD { };

    class epoll
    {
    public:
        using ChannelPtr = std::shared_ptr<channel>;
        using ChanPtrList = std::vector<ChannelPtr>;

        epoll();
        epoll( const epoll& ) = delete;
        epoll& operator=( const epoll& ) = delete;
        ~epoll() { close(epfd_); } // move func will not create

        void update(EPOLL_ADD,ChannelPtr channel,int timeout);
        void update(EPOLL_DELETE,ChannelPtr channel);
        void update(EPOLL_MOD,ChannelPtr channel,int timeout);

        void handleExpried(){ timer_.handleExpried(); }

        ChanPtrList wait();
    private:
        int epfd_;
        timer timer_;
        std::vector<epoll_event> ActiveEvents; // for epoll_wait
        std::map<int,ChannelPtr> channelmap;
    };
} // end summer