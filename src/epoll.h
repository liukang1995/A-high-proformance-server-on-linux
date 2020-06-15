// epoll为epoll*系列的封装，它持有一个epfd，创建、修改、删除，它的生命周期和eventloop一样，为eventloop的持有类。

#ifndef ROOT_SRC_EPOLL_H
#define ROOT_SRC_EPOLL_H

#include "sys/epoll.h"

#include "channel.h"

#include <map>
#include <memory>
#include <vector>

namespace summer
{

    class Epoll 
    {
    private:
        // 持有的epoll文件描述符，在初始化时创建
        int epfd_;

        typedef std::shared_ptr<channel> Channel_Ptr;
        typedef std::vector<Channel_Ptr> Channel_List;

        // 用于保存epoll_wait()活动的事件
        std::vector<epoll_event> active_list;

        // 保存epfd监听的文件描述符
        std::map<int, Channel_Ptr> channels;

    public:
        Epoll();
        ~Epoll();

        // 添加监听fd
        void epoll_add(Channel_Ptr fdptr, int timeout );

        // 修改fd
        void epoll_mod(Channel_Ptr fdptr, int timeout );

        // 删除fd
        void epoll_rm( Channel_Ptr fdptr );

        std::vector<Channel_Ptr> wait();

        int getEpfd() { return epfd_; }
    };

}


#endif