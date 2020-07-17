// author : liukang

#include <chrono>
#include <string>
#include <thread>

#include "threadpool.h"
#include "server.h"

#include "stdio.h"
#include "unistd.h"
#include "netdb.h"
#include <arpa/inet.h>
#include "sys/socket.h"

using namespace std;
using namespace summer;

namespace summer
{
    struct runid_t
    {
        uint32_t addr;
        uint32_t port;
        uint64_t time;
        uint32_t pid;
    };

    auto creatRunid(int port)
    {
        auto pid = getpid();
        auto t = time(NULL);

        char hostname[100];
        gethostname(hostname,100);
        struct hostent* he = gethostbyname(hostname);
        string addr = inet_ntoa( *(struct in_addr*)he->h_addr_list[0] );
        uint32_t ipaddr;
        for( int first = 0, followed = 0; first <= 32; first++ )
        {
            if(first == 32 || addr[first] == '.')
            {
                string num = addr.substr(followed, first-followed);
                auto n = stoi(num);
                ipaddr = ipaddr * 256 + n;
                followed = first+1;
            }
        }
        struct runid_t id;
        id.addr = ipaddr;
        id.port = port;
        id.time = t;
        id.pid = pid;
        return id;
    }
} // end summer

int main( int argc, char* argv[] )
{
    // step 1: init server struction
    
    auto thread_num = thread::hardware_concurrency();
    int port = 80;

    // step 2: load config 
    // 1、command paraments
    int ch;
    while( (ch = getopt(argc,argv,"t:p:")) != -1)
    {
        switch (ch)
        {
        case 't':
            thread_num = stoi( string(optarg) );
            break;
        case 'p':
            port = stoi( string(optarg) );
            break;
        default:
            break;
        }
    }

    // 2、load form config file
    /*


    */
    auto run_id = creatRunid( port );

    // step 3: init server data struct
    threadpool pool( thread_num );
    eventloop loop; // main thread loop
    shared_ptr<server> server_( new server(&loop, thread_num) );
    server_->pool_ = &pool;
        // register server
    loop.queueinloop([server_]()
                    {
                        server_->getloop()->getepoll()->update(EPOLL_ADD(),server_->getchannel(),0);
                    });

    // step 4: exec event loop
    pool.start();
    loop.loop();

    return 0;
}