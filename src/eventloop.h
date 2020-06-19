// eventloop代表了一个线程的主循环

#ifndef ROOT_SRC_EVENTLOOP_H
#define ROOT_SRC_EVENTLOOP_H

#include "channel.h"
#include "epoll.h"

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
        typedef std::function<void()> functor;

        eventloop();
        ~eventloop();

        void loop();
        void quit();

        void runInLoop( functor&& );
        void queueInloop( functor&& );

        bool isInloopthread() { return std::this_thread::get_id() == ThreadID_ ; }

        bool shutdown(std::shared_ptr<channel> channnel);

        // 从epoller中更新、删除、添加channel
        void remove(std::shared_ptr<channel> channel);
        void update(std::shared_ptr<channel> channel);
        void add(std::shared_ptr<channel> channel);

        void wakeup();

    private:
        // 控制循环条件
        bool looping_;

        // 一个evnetloop对应一个epoll，考虑改为composition
        std::shared_ptr<Epoll> epoller_;

        // eventfd,用于异步唤醒
        int wakeupfd_;
        std::shared_ptr<channel> wakeupchannel_;

        bool quit_;

        bool eventHanding_;

        // 异步传递可执行对象
        std::mutex mutex_;
        std::vector<functor> pendingFunctors;
        bool CallingPendingFunctors;

        std::thread::id ThreadID_;

        void doPendingFunctors();

        void handleRead();
        void handleConnect();
    };
}

#endif