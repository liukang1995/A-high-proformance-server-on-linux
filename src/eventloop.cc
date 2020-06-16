


#include "eventloop.h"

#include "sys/eventfd.h"
#include "assert.h"
#include "sys/unistd.h"
#include "stddef.h"  // for size_t

#include <sstream>
#include <thread>

using namespace std;
using namespace summer;

eventloop::eventloop()
    :looping_(false),
    epoller_(new Epoll()),
    wakeupfd_( eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC) ),
    wakeupchannel_( new channel(this, wakeupfd_) ),
    quit_(false),
    eventHanding_(false),
    mutex_(),
    CallingPendingFunctors(false),
    pendingFunctors(),
    ThreadID_( this_thread::get_id() )
{
    assert(wakeupfd_>0);

    //边缘触发
    wakeupchannel_->setEvents(EPOLLIN | EPOLLET);
    wakeupchannel_->setreadcallback( bind(&eventloop::handleRead,this) );
    

    /*



    set connectcallback?



    */

   add(wakeupchannel_);
}

eventloop::~eventloop()
{
    wakeupchannel_->setEvents(0);
    update(wakeupchannel_);
    remove(wakeupchannel_);
    close(wakeupfd_);
}

void eventloop::quit()
{
    quit_ = true;
    if( !isInloopthread() )
        wakeup();
}

void eventloop::wakeup()
{
    uint64_t word = 1;

    //向wakeupfd写入数据
    size_t written = 0;
    size_t writte_left = sizeof(word);

    while( written < sizeof(word) )
    {
        int n = write(wakeupfd_, static_cast<char*>( static_cast<void*>(&word) )+written, writte_left );

        writte_left -= n;
        written += n;
    }

    /*


    EINTER,EAGAIN 处理？


    */
    assert( written == sizeof(word) );
}


/*





有问题，当读多个数据时可以的




*/

//提供给eventfd的读回调
void eventloop::handleRead()
{
    uint64_t word;

    size_t readed = 0;
    char* dst = static_cast<char*>( static_cast<void*>(&word) );

    while( readed < sizeof(word) )
    {
        int n = read(wakeupfd_, dst + readed,sizeof(word) );
        readed += n;
    }
    /*



    handle errno?


    */
   assert( readed == sizeof(word) );
}

void eventloop::runInLoop( functor&& cb )
{
    if( isInloopthread() )
        cb();
    else
    {
        queueInloop(std::move(cb));
    }
    
}

void eventloop::queueInloop( functor&& cb )
{
    unique_lock<mutex> lck(mutex_);
    pendingFunctors.push_back(cb);
    lck.unlock();

//唤醒线程的两种情况：
// 1、此函数由其他线程调用
// 2、线程正在调用functor，此时需要等待下个循环

    if (!isInloopthread() || CallingPendingFunctors )
        wakeup();
}

// not thread safe
void eventloop::doPendingFunctors()
{
    if(  !pendingFunctors.empty() )
    {
        std::vector<functor> functors;
        
        // 此时，若其他线程再向pendingfunctors加入，则放入下一次
        CallingPendingFunctors = true;
        {
            lock_guard<mutex> lck(mutex_);
            functors.swap(pendingFunctors);
        }
        for( auto& f: functors )
            f();

        CallingPendingFunctors = false;
    }
}

void eventloop::loop()
{
    //标志线程进入循环
    looping_ = true;

    //控制线程主循环
    quit_ = false;

    //存放epoll_wait返回的值的channel
    std::vector<Epoll::Channel_Ptr> channels;

    while( !quit_ )
    {
        channels.clear();
        channels = epoller_->wait();

        // 处理事件
        eventHanding_ = true;
        for ( auto& cha: channels )
            cha->handleEvents();
        eventHanding_ = false;

        doPendingFunctors();
        
        /*




        处理过期事件



        */
    }
    looping_ = false;
}