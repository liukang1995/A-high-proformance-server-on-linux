// timernode: 一个计时器节点，它代表一个文件描述符和它关联的超时值
// timermanager: 管理timernode，取出超时节点，并执行超时任务
#ifndef ROOT_SRC_TIME_H
#define ROOT_SRC_TIME_H

#include "httpconnection.h"

#include <time.h>

#include <functional>
#include <memory>
#include <queue>

namespace summer
{
    class timernode
    {
    public:
        typedef std::weak_ptr<httpconnection> httpconnectionPtr;

        timernode( httpconnectionPtr data, std::function<void()> timeProc,time_t timeout,unsigned int id );
        // timernode( const timernode& rhs );

        // enable copy
        timernode(const timernode&) = default;
        timernode& operator=(const timernode&) = default;

        // enable move
        timernode( timernode&& ) = default;
        timernode& operator=( timernode&& ) = default;
        
        // default dest
        ~timernode() = default;

        void process() { timeProc_(); }

        void update( time_t timeout );
        bool isValid();

        void clear() { httpconnection_.reset(); deleted_ = true; }

        // 主动删除
        void setdeleted() { deleted_ = true; }
        bool isdeleted() { return deleted_; }

        time_t getExpired() { return when_; }

    private:
        bool deleted_;
        unsigned int id_;
        time_t when_;
        std::function<void()> timeProc_;
        std::weak_ptr<httpconnection> httpconnection_;
    };


    class timerManager
    {
    public:
        timerManager():id(0){ }
        ~timerManager() = default;

        typedef std::shared_ptr<timernode> TimeNodePtr;
        using WeHttpConnPtr = std::weak_ptr<httpconnection>;

        struct comp{
            bool operator()( TimeNodePtr& lhs, TimeNodePtr& rhs)
            {
                return lhs->getExpired() > rhs->getExpired();
            }
        };

        void addtimer(WeHttpConnPtr data,time_t timeout,std::function<void()> timeProc);
        void handleExpired();
    private:
        int id;
        std::priority_queue<TimeNodePtr, std::deque<TimeNodePtr>, comp> nodes;
    };

} // end summer


#endif