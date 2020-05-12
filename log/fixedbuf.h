// a fixed buffer
// 4k大小用于缓存当前线程日志
// 4m大小用于日志线程先后端缓冲

#ifndef ROOT_LOG_FIXEDBUFFER_H
#define ROOT_LOG_FIXEDBUFFER_H

#include <string>

namespace summer{

  namespace log{

    const int ksmallbuf = 4 * 1024;
    const int klargebuf = 4 * 1024 * 1024;

template <int SIZE>
class FixedBuffer
{
 FixedBuffer( )
   :cur_(buf_)
  {

  }
  ~FixedBuffer( );

  int append( const char*, int );
  char* data();
  int length();
  char* current();
  int avail();
  void bzero();
  std::string tostring();

private:

  char buf_[SIZE];
  char* cur_;
};


 template class FixedBuffer<ksmallbuf>;
 template class FixedBuffer<klargebuf>;
  } //namespace log
} //namespace summer

#endif
