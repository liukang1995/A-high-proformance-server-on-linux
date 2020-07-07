

#include "unistd.h"

#include "server.h"
#include "eventloop.h"
#include <thread>

using namespace std;
using namespace summer;

int main(int argc,char* argv[])
{
    auto thread_num = thread::hardware_concurrency();

    if( thread_num == 0 )
        thread_num = 1;

    eventloop loop;
    server Server(&loop,thread_num,80);
    Server.start();
    loop.loop();
    return 0;
}