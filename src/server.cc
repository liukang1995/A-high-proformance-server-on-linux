

#include "server.h"
#include "httpconnection.h"

#include "fcntl.h"  //for fcntl()
#include "string.h" // for memset
#include "unistd.h"
#include "netinet/in.h"
#include "netinet/tcp.h" // for TCP_NODELAY
#include "sys/socket.h" // socket setsockopt
#include "sys/types.h" //not need
#include "signal.h"

#include "stdio.h"

using namespace std;
using namespace summer;

namespace summer
{
    int CreateListenedFd( int port )
    {
        if( port < 0 || port > 65535 ) return -1;

        int listen_fd = socket( AF_INET, SOCK_STREAM, 0);
        if( listen_fd < 0 )
        {
            perror("create fd error");
            return -1;
        }

        int reuse = 1;
        int v = setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
        if( v < 0 )
        {
            perror("setsockopterr");
            close(listen_fd);
            return -1;
        }

        struct sockaddr_in serveraddr;
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons( static_cast<unsigned short>(port) );
        serveraddr.sin_addr.s_addr = htonl( INADDR_ANY );

        v = bind(listen_fd,(const struct sockaddr*)&serveraddr,sizeof(serveraddr));
        if ( v == -1 )
        {
            perror("bind error");
            close(listen_fd);
            return -1;
        }

        v = listen(listen_fd, 2048);
        if( v == -1 )
        {
            perror( "listen error" );
            close(listen_fd);
            return -1;
        }
        return listen_fd;
    }
} // end summer

server::server(eventloop* loop, int numThread,int port)
    :
    loop_(loop),
    threadNum_(numThread),
    threadpool_( new eventloopthreadpool(loop_,threadNum_) ),
    port_(port),
    listenfd_( CreateListenedFd( port ) ),
    started_(false),
    acceptChannel_( new channel(loop_, listenfd_) )
{
    // ignore sigpipe
    signal( SIGPIPE, SIG_IGN );
    // set listenfd_ nonblock
    int flag = fcntl(listenfd_,F_GETFL,0);
    flag |= O_NONBLOCK;
    fcntl(listenfd_,F_SETFL,flag);
}

void server::start()
{
    threadpool_->start();

    acceptChannel_->setEvents( EPOLLIN | EPOLLET );
    loop_->queueInloop([this]
                    { loop_->update(acceptChannel_,0); }
                    );
    acceptChannel_->setreadcallback( [this]{ handleNewConn();} );
    loop_->add(acceptChannel_,0);
}

// accept一个客户端描述符，并且将其注册到相应的loop上。
void server::handleNewConn()
{
    struct sockaddr_in ClientAddr;
    memset(&ClientAddr,0,sizeof(ClientAddr));
    socklen_t AddrLen = sizeof(ClientAddr);
    while( int fd = accept(listenfd_,(struct sockaddr*)&ClientAddr,&AddrLen))
    {
        eventloop* loop = threadpool_->getnextloop();
        if( fd > 10'0000 ){
            close(fd);
            continue;
        }

        // set nonblock
        int flag = fcntl(fd,F_GETFL,0);
        flag |= O_NONBLOCK;
        fcntl(fd,F_SETFL,flag);

        // no nagle
        int delay = 1;
        setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(const void*)&delay,sizeof(delay));

        shared_ptr<httpconnection> NHConn( new httpconnection(loop,fd) );
        NHConn->getchannel()->setholder(NHConn);
        clients[fd] = NHConn;
        NHConn->addtoloop();
    }
    acceptChannel_->setEvents( EPOLLIN | EPOLLET );
}

