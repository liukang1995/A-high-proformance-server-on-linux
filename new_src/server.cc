#include "server.h"

#include "string.h"
#include "fcntl.h"
#include "sys/socket.h" // for socket setsockopt
#include "netinet/in.h" // for scokaddr_in, htol, htos
#include "netinet/tcp.h"

using namespace std;
using namespace summer;

eventloop* getnextloop();

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

        // reuse addr
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
        serveraddr.sin_port = htons( static_cast<uint16_t>(port) );
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

server::server(eventloop* loop, int port)
    :
    loop_(loop),
    port_(port),
    listenfd_( CreateListenedFd(port_) ),
    acceptChannel_( new channel(loop,listenfd_) ),
    clients()
{

}

// accept一个客户端描述符，并且将其注册到相应的loop上。
void server::handleNewConn()
{
    struct sockaddr_in ClientAddr;
    memset(&ClientAddr,0,sizeof(ClientAddr));
    socklen_t AddrLen = sizeof(ClientAddr);
    while( int fd = accept(listenfd_,(struct sockaddr*)&ClientAddr,&AddrLen))
    {
        eventloop* loop = getnextloop();
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

        shared_ptr<client> NHConn( new client(loop,fd,this) );
        NHConn->getchannel()->SetClient(NHConn);
        clients[fd] = NHConn;
        // 由loop所在线程执行注册任务
        loop->queueinloop( [NHConn]()
                        {
                            NHConn->regist();
                        });
    }
    acceptChannel_->SetEvents( EPOLLIN | EPOLLET );
}
