#include "logthread.h"
#include "logfile.h"

#include <functional>
#include <memory>

using namespace std;
using namespace summer::log;

logthread::logthread( string basename, off_t rollsize )
:   basename_(basename),
    rollsize_(rollsize),
    running_(false),
    cur_buf( new FixedBuffer<klargebuf>() ),
    next_buf( new FixedBuffer<klargebuf>() ),
    bufs_(),
    mx_( ),
    cv_(),
    start_(),
    start_f( start_.get_future() ),
    logthread_( bind( &logthread::LogFunc, this ) )
{

}

// 提供给其他线程调用
void logthread::append( char* logline, int len )
{
    unique_lock<mutex> lock(mx_);

    if ( cur_buf->avail() > len )
        cur_buf->append( logline, len );
    else{

        bufs_.push_back( move(cur_buf) );

        if ( next_buf )
        {
            cur_buf = move( next_buf );
        }
        else
        {
            cur_buf.reset( new FixedBuffer<klargebuf>() );
        }
        cur_buf->append( logline, len );

// 若日志线程被阻塞在条件变量上，通知
        lock.unlock();
        cv_.notify_one();
    }
}

// 启动线程，使其从future::get()阻塞上离开
void logthread::start()
{
    running_ = true;
    start_.set_value(true);
}

//若线程阻塞在cond上，唤醒，退出，资源回收
void logthread::stop()
{
    running_ = false;
    cv_.notify_one();
    logthread_.join();
}

// 提供给线程的函数
void logthread::LogFunc(  )
{
    start_f.get();
    logfile file( basename_, rollsize_ );

    // 用于交换前后端的数据
    Front_Bufs back_bufs;

    // 用于替换前端的缓冲区
    buf_ptr newbuf1( new FixedBuffer<klargebuf> );
    buf_ptr newbuf2( new FixedBuffer<klargebuf> );
    newbuf1->bzero();
    newbuf2->bzero();

    while (running_)
    {
        // 在接下来的临界区内会交换前后端缓冲区，并重置cur_buf,next_buf
        // 等待back_bufs有数据可写
        unique_lock<mutex> lck(mx_);
        cv_.wait_for(lck, chrono::seconds(3),[ & ]{ !back_bufs.empty();});

        bufs_.push_back( move( cur_buf ) );
        cur_buf = move( newbuf1 );

        back_bufs.swap( bufs_ );

        if ( !next_buf )
            next_buf = move( newbuf2 );

        lck.unlock();

        // 将back_bufs内的所有内容写至文件
        int sz = back_bufs.size();
        for ( int i = 0; i < sz; i++ )
        {
            file.append( back_bufs[i]->data(), back_bufs[i]->length() );
        }

        // 重置后端提供的两个缓冲区
        if ( !newbuf1 ){
            newbuf1 = move( back_bufs.back() );
            back_bufs.pop_back();
            newbuf1.reset();
        }

        if ( !newbuf2 ){
            newbuf2 = move( back_bufs.back() );
            back_bufs.pop_back();
            newbuf2.reset();
        }

        back_bufs.clear();
        file.flush();
    }

    file.flush();
}