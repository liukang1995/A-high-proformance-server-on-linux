#include "eventloop.h"

#include "sys/eventfd.h"
#include "sys/epoll.h"
#include "unistd.h"

#include "stdint.h"

using namespace std;
using namespace summer;

eventloop::eventloop()
    :
    looping_(false),
    quit_(false),
    eventHanding_(false),
    Thread_Id_( std::this_thread::get_id() ),
    epoller_(new epoll() ),
    wakefd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
    wakeupchannel_( new channel(this,wakefd_) ),
    mx_(),
    callingfunctors_(false),
    pendfunctors()
{
    wakeupchannel_->SetEvents( EPOLLIN | EPOLLET );
    wakeupchannel_->setReadFunc( [this]{ wakefdRead(); });
    epoller_->update(EPOLL_ADD(),wakeupchannel_,0);
}

eventloop::~eventloop()
{
    wakeupchannel_->SetEvents(0);
    epoller_->update(EPOLL_DELETE(),wakeupchannel_);
    close(wakefd_);    
}

// may be called by other therad
void eventloop::quit()
{
    quit_ = true;
    if( std::this_thread::get_id() != Thread_Id_ )
        wakeup();
}

void eventloop::runinloop( func&& cb )
{
    if( this_thread::get_id() != Thread_Id_ )
        cb();
    else
    {
        queueinloop(move(cb));
    }    
}

void eventloop::queueinloop( func&& cb )
{
    {
        unique_lock<mutex> lck(mx_);
        pendfunctors.push_back(cb);
    }

//唤醒线程的两种情况：
// 1、此函数由其他线程调用
// 2、线程正在调用functor，此时需要等待下个循环

    if (this_thread::get_id() != Thread_Id_ || callingfunctors_ )
        wakeup();
}

void eventloop::wakeup()
{
    uint64_t word = 1;
    int writtenbytes = write(wakefd_,  static_cast<void*>(&word), sizeof(uint64_t) );
    if( writtenbytes != sizeof(word) )
        perror("wakeup fd write error");
}

void eventloop::wakefdRead()
{
    uint64_t word;
    //may be more than one thread wakeup loop
    auto readbytes = read(wakefd_,static_cast<void*>(&word),sizeof(uint64_t));    
}

void eventloop::Procfunctors()
{
    if( !pendfunctors.empty() )
    {
        std::vector<func> functors;

        callingfunctors_ = true;
        {
            unique_lock<mutex> lck(mx_);
            functors.swap(pendfunctors);
        }
        callingfunctors_ = false;
    }
}

void eventloop::loop()
{
    looping_ = true;
    quit_ = false;

    std::vector<epoll::ChannelPtr> channels;
    while( !quit_ )
    {
        channels.clear();
        channels = epoller_->wait();

        eventHanding_ = true;
        for( auto& ch : channels)
            ch->handleEvents();
        eventHanding_ = false;
        Procfunctors();
        epoller_->handleExpried();
    }
    looping_ = false;
}