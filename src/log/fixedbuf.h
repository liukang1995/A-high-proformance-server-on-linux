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

public:
 FixedBuffer( )
   :cur_(buf_)
  {

  }
  ~FixedBuffer( );

//用于向缓冲区添加数据
  int append( const char*, int );

  char* data();
  int length();
  char* current();
  
  int avail();
  void bzero();

  std::string tostring();
  void reset(){ cur = buf_; }

//允许直接向缓冲区添加内容、此为调整cur_指针
  void add( int len ){ cur += len; }

private:

  char buf_[SIZE];
  //始终指向下一个可以填充的位置
  char* cur_;
};

//手动具现化模板

 template class FixedBuffer<ksmallbuf>;
 template class FixedBuffer<klargebuf>;
  } //namespace log
} //namespace summer

#endif
