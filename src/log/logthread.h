// 日志线程与相关数据类


#ifndef ROOT_SRC_LOG_LOGTHREAD_H
#define ROOT_SRC_LOG_LOGTHREAD_H

#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <future>

#include "fixedbuf.h"

namespace summer
{
    namespace log
    {
        class logthread
        {
        public:
            logthread( std::string basename, off_t rollsize );
            ~logthread();

        // thread safe ,在logger的析构函数里调用，他会将数据写入4M大小的前段缓冲区
            void append( char* logline, int len );

        // 启动线程
            void start();

        // 停止线程
            void stop();
        private:
            void LogFunc(  );

        // 控制日志线程启动
            std::promise<bool> start_;
            std::future<bool> start_f;

        //线程循环标志
            std::atomic<bool> running_;

        
            std::string basename_;
            off_t rollsize_;

        //日志线程
            std::thread logthread_;

        //用于同步前后端读写
            std::mutex mx_;
            std::condition_variable cv_;

            typedef std::vector<std::unique_ptr<FixedBuffer<klargebuf>>> Front_Bufs;
            typedef std::vector<std::unique_ptr<FixedBuffer<klargebuf>>>::value_type buf_ptr;

        //前端的两个缓冲区
            buf_ptr cur_buf;
            buf_ptr next_buf;
            
        // 存放已经写满的缓冲区
            Front_Bufs bufs_;
        };



    } // namespace log
} // namespace summer





#endif