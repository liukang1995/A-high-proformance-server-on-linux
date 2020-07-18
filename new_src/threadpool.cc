#include "threadpool.h"

using namespace std;
using namespace summer;

threadpool::threadpool(int numthread)
    :
    start_(false),
    next_(0),
    numthreads_(numthread),
//    baseloop_(baseloop),
    loops_(),
    threads_()
{

}

// called by server
void threadpool::start()
{
    start_ = true;
    for( int i = 0; i < numthreads_; i++ )
    {
        shared_ptr<loopthread> elt( new loopthread() );
        threads_.push_back(elt);
        loops_.push_back( elt->start() );
    }
}

// called by owner
// 如果线程池中没有线程，则返回主loop，否则返回线程池中的下一loop
summer::eventloop* threadpool::getnextloop()
{
    eventloop* loop = nullptr;
    if( !loops_.empty() )
    {
        loop = loops_[next_];
        next_ = ( next_ + 1 ) % numthreads_;    
    }
    return loop;
}