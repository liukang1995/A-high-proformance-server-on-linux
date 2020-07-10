// author: liukang
// timermanager管理注册计时器

#pragma once

#include "time.h"

#include <deque>
#include <functional>
#include <memory>
#include <queue>


namespace summer
{
    class client;
    class timenode
    {
    public:
        using wk_client = std::weak_ptr<client>;
        using timeProc = std::function<void()>;
        
        //desoty,copy,move all default
        timenode(int id, time_t when,wk_client owner,timeProc proc);
        
        void cancel( ) { deleted_ = false; owner_client_.reset(); }
        bool isInvalid( ); //是否超时或被删除
        void process(); //若被主动删除则什么都不做，若正常超时执行回调

        time_t getExp() { return when_; }
    private:
        int id_;
        bool deleted_; // 主动删除
        bool cyclical_;
        timeProc proc_;
        time_t when_;
        wk_client owner_client_;
    };

    // non-local static variate, can not use in other cpp
    auto comp = [](auto lhs,auto rhs)
                {
                    return lhs->getExp() > rhs->getExp();
                };

    class timer
    {
    public:
        using wk_client = std::weak_ptr<client>;
        using timeProc = std::function<void()>;
        using nodePtr = std::shared_ptr<summer::timenode>;

        /*
        defult dest,copy,move
        */
        timer():timenodeid_(0),nodes(comp){ }

        void add(time_t timeout,wk_client ClientName,timeProc proc);
        void handleExpried();

        auto top() { if(!nodes.empty()) return nodes.top(); else return nodePtr(); }
        
    private:
        int timenodeid_;
        std::priority_queue<nodePtr,std::deque<nodePtr>,decltype(comp)> nodes;
    };



} // end summer