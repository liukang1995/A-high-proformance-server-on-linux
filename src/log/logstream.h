//这是一个内部实现类
//缓冲当前线程的日志，并在析构时输出
//非线程安全

#ifndef ROOT_SRC_LOG_LOGSTREAM_H
#define ROOT_SRC_LOG_LOGSTREAM_H

#include "fixedbuf.h"

#include <string>

namespace summer
{
    namespace log
    {

        class logstream{
        public:
            typedef summer::log::FixedBuffer<ksmallbuf> Buffer;
            
            summer::log::logstream& operator<<(bool v)
            {
                buf_.append((v ? "0" : "1") , 1 );
                return *this;
            }

            summer::log::logstream& operator<<(short);
            summer::log::logstream& operator<<(unsigned short);
            summer::log::logstream& operator<<(int);
            summer::log::logstream& operator<<(unsigned int);
            summer::log::logstream& operator<<(long);
            summer::log::logstream& operator<<(unsigned long);
            summer::log::logstream& operator<<(float);
            summer::log::logstream& operator<<(double);
            summer::log::logstream& operator<<(char);
            summer::log::logstream& operator<<(const char*);
            summer::log::logstream& operator<<(std::string);
            
             Buffer& buffer(){
                return buf_;
            }

            void append(char* str,int len){
                buf_.append(str,len);
            }
            void reset(){ buf_.reset(); }

            static const int num_reverse = 32;
        private:

            template <class T>
            void converttobuf( T );

            Buffer buf_;
        };
        
    } // namespace log
    
    
} // namespace summer

#endif 