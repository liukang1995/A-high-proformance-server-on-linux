#include "epoll.h"

#include "stdio.h"

#include "time.h"

using namespace std;
using namespace summer;

epoll::epoll()
    :epfd_( epoll_create1(EPOLL_CLOEXEC) ),
    timer_(),
    ActiveEvents(4096), // alloc advanced
    channelmap()
{
    if( epfd_ < 0 )
        perror(" create epfd error");
}

void epoll::update(EPOLL_ADD,ChannelPtr chan,int timeout)
{
    int fd = chan->getfd();

    if( channelmap.find(fd) != channelmap.end() )
        perror("add error");

    auto client = chan->GetClient();
    if( timeout > 0 )
        timer_.add(timeout,client,[ptr = client.lock()] // attention client lifetime
                    {
                        if( ptr )
                            ptr->handleclose(); //should add client header
                    });

    struct epoll_event event;
    event.data.fd = fd;
    event.events = chan->GetEvents();

    channelmap[fd] = chan;
    if ( epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &event ) < 0 ){
        perror("epoll ctl error");
    }
}

void epoll::update(EPOLL_MOD,ChannelPtr chan,int timeout)
{
    int fd = chan->getfd();

    if( channelmap.find(fd) == channelmap.end() ){
        perror("mod error");
        return;
    }
    auto client = chan->GetClient();
    if( timeout > 0 )
    {
        auto p = client.lock();
        if(p->GetTimernode().lock()) p->GetTimernode().lock()->cancel();  // client should have this func
        timer_.add(timeout,client,[ptr = client.lock()] // attention client lifetime
                    {
                        if( ptr )
                            ptr->handleclose(); //should add client header
                    });
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = chan->GetEvents();

    if ( epoll_ctl( epfd_, EPOLL_CTL_MOD, fd, &event) < 0 ){
        perror("ctl error");
    }
}

void epoll::update(EPOLL_DELETE,ChannelPtr chan)
{
    int fd = chan->getfd();

    if( channelmap.find(fd) == channelmap.end() ){
        perror("delete error");
        return;
    }
    auto client = chan->GetClient().lock();
    if( !client )
        perror(" delete error ");
    auto timenode = client->GetTimernode().lock()->cancel();

    struct epoll_event event;
    event.data.fd = fd;
    event.events = chan->GetEvents();    

    if ( epoll_ctl( epfd_, EPOLL_CTL_DEL, fd, &event) < 0 )
    {
        perror(" delete error");
        return;
    }
    channelmap.erase(fd);    
}

epoll::ChanPtrList epoll::wait()
{
    int timeout;
    auto t = timer_.top();
    if( t )
        timeout = t->getExp() - time(NULL);
    else
        timeout = -1;

    auto count = epoll_wait( epfd_, &*ActiveEvents.begin(),4096,timeout);
    if( count < 0 )
        perror( "epoll_wait err");

    ChanPtrList active_channels;
    for( int i = 0; i < count; i++ )
    {
        int fd = ActiveEvents[i].data.fd;
        ChannelPtr p = channelmap[fd];
        p->SetRevents( ActiveEvents[i].events );
        active_channels.push_back(p);
    }
    return active_channels;
}