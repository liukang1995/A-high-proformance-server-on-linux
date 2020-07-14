// author: liukang

#pragma once
#include "channel.h"
#include "eventloop.h"
#include "timer.h"

#include "unistd.h"

#include <functional>
#include <string>
#include <memory>
#include <map>

namespace summer
{
    enum class RateOfProgress
    {
        START_PARSE_REQUEST_LINE,
        START_PARSE_HEADERS,
        PARSING_HEADERS,
        START_PARSE_BODY,
        PARSING_BODY,
        PARSE_FINISH
    };

    enum class ConnectionState
    {
        CONNECTED,
        DISCONNECTING,
        DISCONNECTED
    };
    
    class server;
    class client
    {
        public:
            using cmdFunc = std::function<void(std::string)>;
            client(eventloop* loop, int fd ,server* server);
            ~client( ) { close(fd_); }

            void SetTimenode( std::shared_ptr<timenode> t) {tinode_ = t;}
            void canneltimer();

            void readQueryFromClient(); // 命令请求处理器
            void sendReplyToClient(); // 命令回复处理器

            void TimeoutHandle();

        private:
            int fd_;
            eventloop* loop_;
            server* server_;
            std::shared_ptr<channel> channel_;
            std::string name_;
            std::weak_ptr<timenode> tinode_;

            std::string cmd_;
            std::string uri_;
            std::map<std::string,std::string> headers_;
            
            std::string inbuffer_;
            std::string outbuffer_;

            RateOfProgress pro_;
            ConnectionState state_;

            std::map<std::string,cmdFunc> CommandMap_;

            bool zero_; // read return 0?
            bool error_; 
            static int DEFAULT_EXPIRED_TIME;
            static int DEFAULT_ALIVE_TIME;

            int parseuri();
            int parseheaders();
            int parsebody();
            int analysis();
    };
}