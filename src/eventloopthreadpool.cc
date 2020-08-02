
#include "eventloopthreadpool.h"

#include "assert.h"

using namespace std;
using namespace summer;

eventloopthreadpool::eventloopthreadpool( eventloop* baseloop, int numthreads )
    :
    baseloop_( baseloop ),
    numthreads_( numthreads ),
    started_( false ),
    next_(0)
{
    assert( numthreads_ > 0 );
}

eventloopthreadpool::~eventloopthreadpool()
{

}

// 创建并启动所有线程
void eventloopthreadpool::start()
{
    // 主baseloop调用
    assert( baseloop_->isInloopthread() );

    started_ = true;

    for( int i = 0; i < numthreads_; i++ )
    {
        shared_ptr<eventloopthread> ev ( new eventloopthread() );
        threads_.push_back( ev );
        loops_.push_back( ev->startloop() );
    }

}

// 返回round robin 下一个loop
eventloop* eventloopthreadpool::getnextloop()
{
    assert( baseloop_->isInloopthread() );

    assert(started_);

    eventloop* loop = baseloop_;
    if( !loops_.empty() )
    {
        loop = loops_[next_];
        next_ = ( next_ + 1 ) % numthreads_;
    }

    return loop;
}