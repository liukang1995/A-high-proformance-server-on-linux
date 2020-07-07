// epoll为epoll*系列的封装，它持有一个epfd，创建、修改、删除，它的生命周期和eventloop一样，为eventloop的持有类。

#ifndef ROOT_SRC_EPOLL_H
#define ROOT_SRC_EPOLL_H

#include "sys/epoll.h"

#include "channel.h"
#include "timer.h"

#include <map>
#include <memory>
#include <vector>

namespace summer
{

    class Epoll 
    {
    public:
        using Channel_Ptr = std::shared_ptr<channel> ;
        using Channel_List = std::vector<Channel_Ptr> ;

    private:
        // 持有的epoll文件描述符，在初始化时创建
        int epfd_;
        // as return value of epoll_wait:  active fd
        std::vector<epoll_event> active_list;
        // 保存epfd监听的文件描述符
        std::map<int, Channel_Ptr> channels;
        summer::timerManager manage_;

    public:
        Epoll();
        ~Epoll();

        void epoll_add(Channel_Ptr fdptr, int timeout );
        void epoll_mod(Channel_Ptr fdptr, int timeout );
        void epoll_rm(Channel_Ptr fdptr );

        void handle_expired(){ manage_.handleExpired(); }

        //核心
        std::vector<Channel_Ptr> wait();

        int getEpfd() { return epfd_; }
    };

}


#endif