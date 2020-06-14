
#include "timer.h"

using namespace std;
using namespace summer;

timernode::timernode(HttpDataPtr data, time_t timeout)
:deleted(false),
httpdata_(data),
expiredTime( time(NULL) + timeout )
{
    
}