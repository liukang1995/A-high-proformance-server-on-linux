// author : liukang

#pragma once

#include "client.h"
#include "channel.h"
#include "eventloop.h" 

#include <unordered_map>
#include <memory>

namespace summer
{
    class server
    {
        public:
            server(eventloop* loop, int port);
            ~server();

            eventloop* getloop(){ return loop_; }

            void handleNewConn(); // 连接应答处理器
            void closeclient(int clientfd) { clients.erase(clientfd); }
            
        private:
            eventloop* loop_;
            int port_;

            int listenfd_;            
            std::shared_ptr<channel> acceptChannel_;
            std::unordered_map<int,std::shared_ptr<client>> clients;            
    };
} // end summer