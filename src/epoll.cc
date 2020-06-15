// EPOLL 管理监听的fd，wait（）用于监听。 它与EVENTLOOP属于composition关系

#include "epoll.h"
#include "log/logger.h"

#include "assert.h"

using namespace summer;
using namespace std;


Epoll::Epoll()
:epfd_( epoll_create1(EPOLL_CLOEXEC) ),
active_list(4096)
{
    assert( epfd_ > 0 );
}

Epoll::~Epoll( ) { }

void Epoll::epoll_add(Channel_Ptr fdptr, int timeout )
{
    int fd = fdptr->getFd();
    /*

    add timer;



    */

   struct epoll_event event;
   event.data.fd = fd;
   event.events = fdptr->getEvents();

   /*



   update fdptr->event?

   */

  // channels用于管理已经监听的fd
    channels[fd] = fdptr;

    if ( epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &event ) < 0 ){
        LOGERROR << "epoll add error.";
        channels.erase(fd);
    }
}

void Epoll::epoll_mod(Channel_Ptr fdptr, int timeout )
{
    int fd = fdptr->getFd();

    // 必然存在
    assert( channels.find(fd) != channels.end() );

    if ( timeout > 0 )
    {
        /*

        update timeer


        */
    }

    // 需要更新
    if( !fdptr->compare_update_lastevents() )
    {
        struct epoll_event event;
        event.events = fdptr->getEvents();
        event.data.fd = fdptr->getFd();

        if ( epoll_ctl( epfd_, EPOLL_CTL_MOD, fd, &event) < 0 ){
            LOGERROR << " EPOLL MOD ERROR, ADD RESET TO LASTEVENTS ";
            
        }
    }
}

void Epoll::epoll_rm( Channel_Ptr fdptr )
{
    /*

    del timer?

    */

    int fd = fdptr->getFd();
    map<int,Channel_Ptr>::iterator it = channels.find(fd);

    assert(it != channels.end() );

    struct epoll_event event;
    event.events = fdptr->getEvents();
    event.data.fd = fd;

    if ( epoll_ctl( epfd_, EPOLL_CTL_DEL, fd, &event) < 0 )
    {
        LOGERROR << "EPOLL DEL ERR. ";
        return;
    }

    // 内核内产出成功，移除Epoll内的副本
    channels.erase( it );
}

std::vector<Epoll::Channel_Ptr> Epoll::wait()
{
    while( true ){
        // 阻塞等待
        int count = epoll_wait( epfd_, &*active_list.begin(),4096,-1);
        if ( count < 0 )
            LOGERROR << "EPOLL WAIT ERROR. ";
        assert( count >= 0 );

        // 根据active_list返回vector<Channel_ptr> 
        vector<Channel_Ptr> active_ptrs;

        for( int i = 0; i < count; i++ )
        {
            int fd = active_list[i].data.fd;

            Channel_Ptr fd_ptr = channels[fd];
            assert( fd_ptr );

            fd_ptr->setrEvents( active_list[i].events );

            //fd_ptr->setEvents( 0 )?

            active_ptrs.push_back(fd_ptr);
        }
        if ( count > 0 )
            return active_ptrs;
    }

}