
#include "timer.h"

using namespace std;
using namespace summer;

timernode::timernode(HttpDataPtr data, time_t timeout)
:deleted(false),
httpdata_(data),
expiredTime( time(NULL) + timeout )
{

}

timernode::~timernode()
{
    /*

    handleclose?

    */
}

void timernode::update( time_t timeout )
{
    time_t now = time(NULL);
    expiredTime = now + timeout;
}

bool timernode::isValid()
{
    time_t now = time(NULL);
    if ( now < expiredTime )
        return true;
    else{
        setdeleted();
        return false;
    }
}

void timerManager::addtimer( timernode::HttpDataPtr data, time_t timeout )
{
    TimeNodePtr node( new timernode( data, timeout ) );
    nodes.push(node);
    /*


    http_data link timer


    */
}

void timerManager::handleExpired()
{
    while( !nodes.empty() )
    {
        // 惰性删除
        TimeNodePtr cur = nodes.top();
        if ( cur->isdeleted() )
            nodes.pop();
        // 计时器已经过期
        else if( !cur->isValid() )
            nodes.pop();
        else break;
    }
}
