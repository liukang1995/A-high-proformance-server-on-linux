#include "channel.h"
#include "log/logger.h"

using namespace std;
using namespace summer;

channel::channel( eventloop* loop, int fd )
  :  
  fd_(fd),
  loop_(loop),
  events(0),
  revents(0),
  hcptr_(),
  srptr_()
{

}

void channel::handleEvents()
{
    events = 0;

    //相关联的文件描述符被挂断,不在关心任何事件
    //Note that when reading from a channel such as a pipe or a stream socket, 
    //this event merely indicates that the peer closed its end of the channel.  
    //Subsequent reads from the channel will return 0 (end of  file)only after all outstanding data in the channel has been consumed.
    if ((revents & EPOLLHUP) && !(revents & EPOLLIN)) {

      LOGWARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
      events = 0;
      return;
    }

    //文件描述符发生错误，不在关心任何事件
    //Error  condition  happened on the associated file descriptor.  This event is also reported for the write end of a pipe when the read end has been closed. 
    if (revents & EPOLLERR) {
      if (errorcallback_) errorcallback_();
      events = 0;
      return;
    }

    //EPOLLRDHUP: Stream socket peer closed connection, or shut down writing half of connection. 
    if (revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
      if ( readcallback_ )
        readcallback_;
    }

    if (revents & EPOLLOUT) {
      if( writecallback_ )
        writecallback_;
    }
    
}

// 记录events是否发生变动
bool channel::compare_update_lastevents( )
{
  bool equal = ( events == lastevents );
  if ( !equal )
    lastevents = events;

  return equal;
}