#include "loopthread.h"

using namespace std;
using namespace summer;

loopthread::loopthread()
    :
    loop_(nullptr),
    exit_(false),
    start_(false),
    pm_(),
    loopthread_([this]{threadFunc();})
{

}

loopthread::~loopthread()
{
    exit_ = true;
    if( loop_ )
    {
        loop_->quit();
        loopthread_.join();
    }
}

eventloop* loopthread::start()
{
    pm_.set_value(); // control the thread go next
    eventloopcreate_.get_future().wait();
    if( !loop_ )
        perror( "carete loop error"); 
    return loop_;
}

void loopthread::threadFunc()
{
    pm_.get_future().wait();
    eventloop loop;
    loop_ = &loop;
    eventloopcreate_.set_value();

    loop.loop();

    loop_ = nullptr;
}