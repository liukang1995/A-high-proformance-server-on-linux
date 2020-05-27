// 一个临时对象，在析构函数中会将日志输出


#ifndef ROOT_SRC_LOG_LOGGER_H
#define ROOT_SRC_LOG_LOGGER_H

#include <functional>

#include "logstream.h"
#include "string.h"

namespace summer{
    namespace log{

        // 计算
        class basename
        {
        public:
            basename( char* path )
            {
                char* p = strrchr(path, '/');

                if(p){
                data_ = p+1;
                size_ = strlen(data_);
            }
            }

            const char* data() { return data_; }
            int size() { return  size_ ; }
        private:

            const char* data_;
            int size_;
        };

        enum Loglevel{
            DEBUG,
            INFO,
            WARN,
            ERROR,
            NUMOFLEVEL,
        };

        //非线程安全的栈上对象，他会将需要输出的日志缓冲信息放在一个大小为4k的缓冲区上，析构时将其输出
        class logger
        {
        public:
            logger( basename name, int line, Loglevel level );

            ~logger();
            logstream& stream(){ return stream_; }

            static Loglevel loglevel(){ return global_loglevel; }
            static Loglevel setloglevel( Loglevel level ){ global_loglevel = level; }

            typedef std::function<void(char*,int)> OutputFunc;
            typedef std::function<void()> FlushFunc;

            void setOutputFunc( OutputFunc& out){ outputfunc_ = out ;}
            void setFlushFunc( FlushFunc& f){ flushfunc_ = f; }

        private:

            static Loglevel global_loglevel;
            OutputFunc outputfunc_;
            FlushFunc flushfunc_;
            Loglevel level_;
            logstream stream_;
            int line_;
            basename name_;
        };

        #define LOGDEBUG summer::log::logger(__FINE__,__LINE__,DEBUG).stream();
        #define LOGINFO summer::log::logger(__FINE__,__LINE__,INFO).stream();
        #define LOGERROR summer::log::logger(__FINE__,__LINE__,ERROR).stream();
        #define LOGWARN summer::log::logger(__FINE__,__LINE__,WARN).stream();

    } // namespace log
} // namespace summer

#endif