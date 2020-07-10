#include "timer.h"

using namespace std;
using namespace summer;

timenode::timenode(int id, time_t when,wk_client owner,timeProc proc)
    :
    id_(id),
    deleted_(false),
    cyclical_(false),
    proc_(proc),
    when_(when),
    owner_client_(owner)
{

}

bool timenode::isInvalid()
{
    if( deleted_ )
        return true;

    time_t now = time(NULL);
    if( now > when_ )
        return true;

    return false;
}

void timenode::process()
{
    if( deleted_ )
        return;

    time_t now = time(NULL);
    if( now > when_ )
        proc_();

    return;
}

void timer::add(time_t timeout,wk_client ClientName,timeProc proc)
{
    time_t timestamp = time(NULL) + timeout;
    nodePtr p(new timenode(timenodeid_++,timestamp,ClientName,proc));
    nodes.push(p);
    /*
    向client注册p
    */
}

void timer::handleExpried()
{
    if( nodes.empty() )
        return;

    auto root = nodes.top();
    while( !nodes.empty() && root->isInvalid() )
    {
        nodes.pop();
        root->process();
        if( !nodes.empty() )
        {
            root = nodes.top();
        }
    }
}
