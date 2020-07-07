// EPOLL 管理监听的fd，wait（）用于监听。 它与EVENTLOOP属于composition关系

#include "epoll.h"
#include "log/logger.h"

#include "unistd.h"

#include "assert.h"

using namespace summer;
using namespace std;


Epoll::Epoll()
    :epfd_( epoll_create1(EPOLL_CLOEXEC) ),
    active_list(4096),
    manage_()
{
    assert( epfd_ > 0 );
}

Epoll::~Epoll( ) { close(epfd_); }

void Epoll::epoll_add(Channel_Ptr fdptr, int timeout )
{
    int fd = fdptr->getFd();
    
    //timeout秒之后，将fdptr从epoll监听的列表上剥离
    if(timeout > 0)
        manage_.addtimer(fdptr->getholder(),timeout,[ptr = fdptr->getholder().lock()]
                                                {
                                                    if( ptr ){
                                                        ptr->handleclose();
                                                    }
                                                });

    struct epoll_event event;
    event.data.fd = fd;
    event.events = fdptr->getEvents();
    //recode this change, for modify in future
    fdptr->compare_update_lastevents();
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

    // 已在监听
    assert( channels.find(fd) != channels.end() );
    // 需要更新计时器，未生效的timernode执为实效，并重新添加新节点
    if ( timeout >= 0 )
    {
        auto p = fdptr->getholder().lock();
        if( p ){
           auto t = p->gettimer().lock();
           if( t ) t->setdeleted();
           if( timeout > 0)
                manage_.addtimer(fdptr->getholder(),timeout,[ptr = fdptr->getholder().lock()]
                                                    {
                                                        if( ptr ){
                                                            ptr->handleclose();
                                                        }
                                                    });
        }
    }

    // 需要更新事件
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
    auto p = fdptr->getholder().lock();
    if(p)
        p->canceltimer();

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
    channels.erase( it );
}

std::vector<Epoll::Channel_Ptr> Epoll::wait()
{
    while( true ){
        // 阻塞等待
        int count = epoll_wait( epfd_, &*active_list.begin(),4096,-1);
        if ( count < 0 )
            LOGERROR << "EPOLL WAIT ERROR. ";

        // 根据active_list返回vector<Channel_ptr> 
        vector<Channel_Ptr> active_ptrs;

        for( int i = 0; i < count; i++ )
        {
            int fd = active_list[i].data.fd;

            Channel_Ptr fd_ptr = channels[fd];
            fd_ptr->setrEvents( active_list[i].events );
            //fd_ptr->setEvents( 0 )?
            active_ptrs.push_back(fd_ptr);
        }
        if ( count > 0 )
            return active_ptrs;
    }

}