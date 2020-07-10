#include "channel.h"
#include "epoll.h"
#include "sys/epoll.h"

using namespace std;
using namespace summer;

uint32_t DEFAULT_READ_EVENTS = EPOLLIN | EPOLLPRI | EPOLLET;
uint32_t DEFAULT_WRITE_EVENTS = EPOLLOUT;

time_t DEFAULT_TIMEOUT = 2000;


channel::channel(eventloop* loop, int fd)
    :
    fd_(fd),
    loop_(loop),
    events_(DEFAULT_READ_EVENTS),
    revents_(DEFAULT_WRITE_EVENTS)
{

}

void channel::EnableRead()
{
    events_ = DEFAULT_READ_EVENTS;
    auto ep = epoller_.lock();
    if( ep )
    {
        ep->update(EPOLL_MOD(),shared_from_this(),DEFAULT_TIMEOUT);
    }
}

void channel::DisableRead()
{
    events_ ^= DEFAULT_READ_EVENTS;
    auto ep = epoller_.lock();
    if( ep )
    {
        ep->update(EPOLL_MOD(),shared_from_this(),DEFAULT_TIMEOUT);
    }
}

void channel::EnableWrite()
{
    events_ = DEFAULT_WRITE_EVENTS;
    auto ep = epoller_.lock();
    if( ep )
    {
        ep->update(EPOLL_MOD(),shared_from_this(),DEFAULT_TIMEOUT);
    }
}

void channel::DisableWrite()
{
    events_ ^= DEFAULT_WRITE_EVENTS;
    auto ep = epoller_.lock();
    if( ep )
    {
        ep->update(EPOLL_MOD(),shared_from_this(),DEFAULT_TIMEOUT);
    }
}