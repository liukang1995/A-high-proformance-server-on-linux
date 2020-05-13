// 一个固定大小的buffer类
// 非继承类

#ifndef ROOT_LOG_FIXEDBUFFER_H
#define ROOT_LOG_FIXEDBUFFER_H

#include <string>

template <int SIZE>
class FixedBuffer
{
  FixedBuffer( );
  ~FixedBuffer( );

  int append( const char*, int len );
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


#endif
