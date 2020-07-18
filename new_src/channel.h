// author: liukang
// channel is a io dispenser

#pragma once

#include "stdint.h"
#include "client.h"
#include "epoll.h"
#include "eventloop.h"

#include <functional>
#include <memory>

namespace summer
{
    // require channel object managed by shared_ptr before share_from_this()
    class channel: public std::enable_shared_from_this<channel>
    {
    public:
        using func = std::function<void()>;
        using WkClientMaster = std::weak_ptr<client>;

        // defalt other special function
        channel( eventloop* loop, int fd );

        auto GetEvents() { return events_; }
        void SetEvents(uint32_t val) { events_ = val; }

        auto GetRevents() { return revents_; }
        void SetRevents(uint32_t val) { revents_ = val; }

        void setReadFunc(func f) { ReadFunc_ = std::move(f); }
        void setWriteFunc(func f) { WriteFunc_ = std::move(f); }

        void EnableRead();
        void DisableRead();

        void EnableWrite();
        void DisableWrite();

        int getfd() { return fd_; }

        void SetClient(WkClientMaster client) { client_ = client; }
        auto GetClient() { return client_; }
        void SetEpoller(std::weak_ptr<epoll> epoller) { epoller_ = epoller; }
        auto GetEpoller() { return epoller_; }

        void handleEvents();
    private:
        int fd_;
        std::weak_ptr<epoll> epoller_;
        eventloop* loop_;
        uint32_t events_;
        uint32_t revents_;

        func ReadFunc_;
        func WriteFunc_;

        WkClientMaster client_;
    };
} // end summer