// server负责监听fd,并将建立好的连接分发至线程池中

#ifndef ROOT_SRC_SERVER_H
#define ROOT_SRC_SERVER_H

#include "eventloop.h"
#include "eventloopthreadpool.h"

#include <memory>

namespace summer
{
    class server{
    public:
        server(eventloop* loop,int threadNum,int port);
        ~server();

        eventloop* getloop();
        void start();
        void handleNewConn();

    private:
        eventloop* loop_;
        int threadNum_;
        std::unique_ptr<eventloopthreadpool> threadpool_;

        int port_;
        int listenfd_;
        bool started_;

        std::shared_ptr<channel> acceptChannel_;
    };
   
} // namespace summer

#endif