//

#include "fixedbuf.h"

#include <string>

#include "string.h"

using namespace std;
using namespace summer;
using namespace summer::log;

template <int SIZE>
int FixedBuffer<SIZE>::append(const char* str, int len)
{
  if ( avail() < len ){
    memcpy(cur_,str,len);
    cur_ += len;
  }
  return len;
}

template <int SIZE>
char* FixedBuffer<SIZE>::data()
{
  return buf_;
}

template <int SIZE>
int FixedBuffer<SIZE>::length()
{
  return static_cast<int>(sizeof(buf_));
}

template <int SIZE>
char* FixedBuffer<SIZE>::current()
{
  return cur_;
}

template <int SIZE>
int FixedBuffer<SIZE>::avail()
{
  char* end = buf_ + length();
  return end - cur_;
}

template <int SIZE>
void FixedBuffer<SIZE>::bzero()
{
  memset(buf_,0,sizeof(buf_));
}

template <int SIZE>
string FixedBuffer<SIZE>::tostring()
{
  return string(buf_);
}
