// 持有file类，负责滚动日志文件

#ifndef ROOT_SRC_LOG_LOGFILE_H
#define ROOT_SRC_LOG_LOGFILE_H

#include "file.h"

#include <memory>
#include <mutex>
#include <string>

namespace summer{

namespace log
{
    
    class logfile
    {
    public:
        logfile( std::string basename, off_t rollsize );

    // 用于向文件中写数据，thread safe
        void append(char* logline, size_t len );

    // 刷新输出缓冲区
        void flush();

    // 用于生成新的日志文件名
        std::string getlogname();

    // 滚动文件
        void rollfile();

    private:
    // 由用户制定的日志名，用于构建日志文件名
        std::string basename_;

    // 当前日志文件已经写入的字节数
        off_t rollsize_;

    // 惰性检查当前是否超时，当累计count次
        const int CheckN_;
        int count;

    // 保护以下三个 time_t读写
        std::mutex mutex_;

        time_t last_roll;
        time_t last_flush;
    
    // 每隔24小时，会强行回滚日志，惰性
        time_t startofperiod;

    // 持有一份file，当滚动后，自动关闭文件流
        std::unique_ptr<summer::log::file> file_;
    };

} // namespace log

} // end summer

#endif