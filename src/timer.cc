
#include "timer.h"

using namespace std;
using namespace summer;

timernode::timernode( httpconnectionPtr data, std::function<void()> timeProc,time_t timeout, unsigned int id )
    :
    deleted_(false),
    id_(id),
    when_( time(NULL) + timeout ),
    timeProc_(timeProc),
    httpconnection_(data)
{

}

void timernode::update( time_t timeout )
{
    time_t now = time(NULL);
    when_ = now + timeout;
}

bool timernode::isValid()
{
    time_t now = time(NULL);
    if ( now < when_ )
        return true;
    else{
        setdeleted();
        return false;
    }
}

void timerManager::addtimer( WeHttpConnPtr data, time_t timeout,std::function<void()> timeProc )
{
    TimeNodePtr node( new timernode( data, timeProc,timeout,++id ) );
    nodes.push(node);
    shared_ptr<httpconnection> SPHttp = data.lock();
    if( SPHttp )
        SPHttp->settimer( node );
}

void timerManager::handleExpired()
{
    while( !nodes.empty() )
    {
        // 惰性删除
        TimeNodePtr cur = nodes.top();
        if ( cur->isdeleted() ){
            nodes.pop();
        }else if( !cur->isValid() ){
            nodes.pop();
            cur->process();
        }else 
            break;
        //退出此空间后，启动超时timenode的析构行为
    }
}
