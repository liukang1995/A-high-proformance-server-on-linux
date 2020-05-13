//这是一个内部实现类
//缓冲当前线程的日志，并在析构时输出
//非线程安全

#ifndef ROOT_SRC_LOG_LOGSTREAM_H
#define ROOT_SRC_LOG_LOGSTREAM_H

#include "fixedbuf.h"

#include <string>

using namespace std;
using namespace summer;
using namespace summer::log;

namespace summer
{
    namespace log
    {

        class logstream{
        public:
            typedef summer::log::FixedBuffer<ksmallbuf> Buffer;
            
            logstream& operator<<(bool v)
            {
                buf_.append((v ? "0" : "1") , 1 );
                return *this;
            }

            logstream& operator<<(short);
            logstream& operator<<(unsigned short);
            logstream& operator<<(int);
            logstream& operator<<(unsigned int);
            logstream& operator<<(long);
            logstream& operator<<(unsigned long);
            logstream& operator<<(float);
            logstream& operator<<(double);
            logstream& operator<<(char);
            logstream& operator<<(char*);
            logstream& operator<<(string);
            
            const Buffer& buffer(){
                return buf_;
            }

        private:
            Buffer buf_;
        };
        
    } // namespace log
    
    
} // namespace summer

#endif 