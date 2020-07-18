#include "client.h"
#include "epoll.h"

#include "string.h"
#include "sys/stat.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/mman.h"

#include <regex>


using namespace std;
using namespace summer;

namespace summer{

int client::DEFAULT_EXPIRED_TIME = 2000;
int client::DEFAULT_ALIVE_TIME = 5*60*1000;

int read2string(int fd, string s, bool &zero)
{
    int readsum = 0;
    int cur_read = 0;

// 读完当前缓冲区所有数据
    while( true )
    {
        char buf[4096];
        if( cur_read = read(fd, buf, 4096) ){
            if ( cur_read == -1 ){
                if( errno == EINTR ){
                    cur_read = 0;
                    continue;
                }
                else if( errno == EAGAIN ){ // 无数据可读
                    return readsum;
                }else{
                    return -1;
                }
            }
        }else{
                // cur_read == 0, 对端发送FIN
                zero = true;
                break;
        }
        readsum += cur_read;
        s += string(buf,buf+cur_read);
    }
    return readsum;
}

int write2fd(int fd, char* data, size_t len)
{
    size_t nleft = len, nwritten = 0, cur_write;

    auto p = data;
    while( nleft > 0 ){
        if( (cur_write = write(fd, data, len)) <= 0 ){
            if ( cur_write < 0 ){
                if( errno == EINTR ){
                    cur_write = 0;
                    continue;
                }
                else if(errno == EAGAIN){
                    return nwritten;
                }else{
                    return -1;
                }
            }
        }
        nwritten += cur_write;
        nleft -= cur_write;
        p += cur_write;
    }
    return nwritten;
}

// 将请求行拆封成3个字符串
vector<string> split(string s)
{
    vector<string> result;
    auto sz = s.size();

    size_t idx = 0;
    while( idx < sz && s[idx] == ' ' )
    {
        idx++;
    }

    size_t beg_str = idx;
    for( ; idx <= sz; idx++ )
    {
        if( idx == sz || s[idx] == ' ' ){
            result.emplace_back( s.substr(beg_str,idx-beg_str) );
            beg_str = idx + 1;
        }
    }

    return result;
}

string getmime( string s )
{
    static map<string,string> mime_map;
    if( mime_map.empty() )
    {
        mime_map[string(".html")] =  string("text/html");
        mime_map[string(".avi")] = string("video/x-mavideo");
        mime_map[string(".bmp")] = string("image/bmp");
        mime_map[string(".c")] = string("text/plain");
        mime_map[string(".doc")] = string("application/msword");
        mime_map[string(".gif")] = string("image/git");
        mime_map[string(".gz")] = string("application/x-gzip");
        mime_map[string(".ico")] = string("image/x-icon");
        mime_map[string(".jpg")] = string("image/jpeg");
        mime_map[string(".png")] = string("image/png");
        mime_map[string(".txt")] = string("text/plain");
        mime_map[string(".mp3")] = string("audio/mp3");
        mime_map[string("default")] = string("text/html");
    }
    if( mime_map.find(s) != mime_map.end() )
        return mime_map[s];
    else
    {
        return mime_map[string("default")];
    }    
}

void SendErrorMessage(int fd, int errnum, string msg)
{
    char buf[4096];
    char n[11];
    char content_size[11];
    snprintf(n,11,"%d",errnum);

    string body_buf, head_buf;
    body_buf += "<html><title>出错了</title>" ;
    body_buf += "<body bgcolor=\"ffffff\">";
    body_buf += buf + msg;
    body_buf += "<em> SEVER ERROR </em>\n</body></html>";

    snprintf(content_size,11,"%d",body_buf.size());
    head_buf += "HTTP/1.1";
    head_buf += n + msg +"/r/n";
    head_buf += "Content-Type: text/html\r\n";
    head_buf += "Connection: Close\r\n";
    head_buf += "Content-Length: ";
    head_buf += content_size;
    head_buf += "\r\n";
    head_buf += "Server: liukang's Web Server\r\n";
    head_buf += "\r\n";

    snprintf(buf,4096,"%s",head_buf.c_str());
    write2fd(fd,buf,strlen(buf));
    snprintf(buf,4096,"%s",body_buf.c_str());
    write2fd(fd,buf,strlen(buf));    
}

} // end summer

client::client(eventloop* loop, int fd ,server* server)
    :
    fd_(fd),
    loop_(loop),
    server_(server),
    channel_(new channel(loop_,fd_) ),
    name_(),
    tinode_(),
    cmd_(),
    uri_(),
    headers_(),
    pro_(RateOfProgress::START_PARSE_HEADERS),
    state_(ConnectionState::CONNECTED),
    zero_(false),
    error_(false)
{
    channel_->setReadFunc([this] { readQueryFromClient(); });
    channel_->setWriteFunc([this] {sendReplyToClient();});
}

void client::canneltimer()
{
    auto t = tinode_.lock();
    if( t )
    {
        t->cancel();
    }
}

// -1:error 0:success 1:again
int client::parseuri()
{
    auto pos = inbuffer_.find("\r\n",0);
    if( pos == string::npos )
        return 1;
    string request_line = inbuffer_.substr(0,pos);
    inbuffer_.erase(0,pos+1);
    auto strs = split(request_line);

    if( strs[0] == "GET" ){
        cmd_ = "GET";
    }else if( strs[0] == "POST" ){
        cmd_ = "POST";
    }else if( strs[0] == "HEAD" ){
        cmd_ = "HEAD";
    }else{
        return -1;
    }

    uri_ = string("/home/lk/Desktop") + strs[1];
    return 0;
}


int client::parseheaders()
{
    while( true )
    {
        auto p = inbuffer_.find("\r\n",0);
        if(p == string::npos)
            return 1;
        
        if( p == 0 ){
            inbuffer_.erase(0,2);
            return 0;}

        string head_line = inbuffer_.substr(0,p);
        inbuffer_.erase(p+1);
        regex rgx("(.*):[[:blank:]]*(.*)");
        smatch MatchResult;
        auto mth = regex_match(head_line,MatchResult,rgx);

        if( !mth )
            return -1;
        headers_[MatchResult.str(1)] = MatchResult.str(2);
    }
}

int client::parsebody()
{
    int content_length;
    if( headers_.find("Content-length") != headers_.end() ){
        content_length = stoi( headers_["Content-lenght"] );
        if( content_length == inbuffer_.size() )
            return 0;
        else{
            return 1;
        }
    }else{
        return -1;
    } 
}

int client::analysis()
{
    auto p = uri_.rfind('.');
    string file_type;
    if( p == string::npos ){
        file_type = getmime("default");
    }else{
        file_type = getmime(uri_.substr(p));
    }

    string header;
    struct stat file_status;
    if( stat(uri_.c_str(), &file_status) < 0 ){
        if( errno == ENOENT ){
            error_ = true;
            SendErrorMessage(fd_,404,"Not Found");
            return -1;
        }else{
            error_ = true;
            SendErrorMessage(fd_,400,"Bad Request");
            return -1;
            }
    }else{
        //存在此文件且可读
        header += "HTTP/1.1 200 OK";
        if ( headers_.find("Connection") != headers_.end() ){
            if(headers_["Connection"] == "Keep-Alive" || headers_["Connection"] == "keep-alive"){
                header += "Connection: Keep-Alive\r\n";
                header += "Keep-Alive: timeout=" + to_string(DEFAULT_ALIVE_TIME) +"\r\n";
            }
        }
            header += "Content-Type: " + file_type + "\r\n";
            header += "Content-Length: " + to_string(file_status.st_size) + "\r\n";
            header += "Server: Liukang's Web Server"; 
    }
    outbuffer_ += header;
    if( cmd_ == "HEAD" )
        return 0;

    int source_fd = open(uri_.c_str(),O_RDONLY);
    if( source_fd < 0 ){
        outbuffer_.clear();
        SendErrorMessage(fd_, 404, "Not Found");
        return -1;
    } 
    void *mR = mmap(NULL,file_status.st_size,PROT_READ,MAP_PRIVATE,source_fd,0);
    close(source_fd);
    if( mR == MAP_FAILED )
    {
        error_ = true;
        munmap(mR,file_status.st_size);
        outbuffer_.clear();
        SendErrorMessage(fd_,404,"Not Found!");
        return -1;
    }   
    outbuffer_ += string( static_cast<const char*>(mR),static_cast<const char*>(mR) + file_status.st_size );
    munmap(mR,file_status.st_size);
    return 0;
}

void client::readQueryFromClient()
{
    int readsum = read2string(fd_,inbuffer_,zero_);

    if( readsum == -1 )
    {
        error_ = true;
        SendErrorMessage(fd_,400,"Server error");
    }

    if( zero_ )
    {
        state_ = ConnectionState::DISCONNECTING;
        canneltimer();
        loop_->queueinloop([this] { TimeoutHandle(); });
        return;
    }

    if( pro_ == RateOfProgress::START_PARSE_HEADERS )
    {
        auto flag = parseuri();
        if( flag == -1 )
        {
            error_ = true;
            SendErrorMessage(fd_,400,"Bad Request");
        }
        if( flag == 1 )
            return;
        if( flag == 0 )
            pro_ = RateOfProgress::START_PARSE_HEADERS;
    }

    if( pro_ == RateOfProgress::START_PARSE_HEADERS )
    {
        auto flag = parseheaders();
        if( flag == -1 )
        {
            error_ = true;
            SendErrorMessage(fd_,400,"Bad Request");
        }
        if( flag == 1)
            return;
        if( flag == 0 )
        {
            pro_ = RateOfProgress::PARSING_BODY;
        }
    }

    if( pro_ == RateOfProgress::START_PARSE_BODY )
    {
        auto flag = parsebody();
        if( flag == -1)
        {
            error_ = true;
            SendErrorMessage(fd_,400,"Bad Request");
        }
        if( flag == 1 )
            return;
        if( flag == 0 )
        {
            pro_ = RateOfProgress::PARSE_FINISH;
        }
    }


    if( pro_ == RateOfProgress::PARSE_FINISH )
    {
        auto flag = analysis();
        if( flag = -1 )
        {
            error_ = true;
        }
        if( flag == 1 )
            return;
    }

    if( error_ )
    {
        perror( " readQueryfromClient error " );
        auto t = tinode_.lock();
        if( t )
        {
            t->cancel();
        }
        loop_->queueinloop( [this]{ TimeoutHandle(); });
        return;
    }

    if( !outbuffer_.empty() )
    {
        sendReplyToClient();
    }

    // read->analysis success
    cmd_.clear();
    uri_.clear();
    inbuffer_.clear();
    pro_ = RateOfProgress::START_PARSE_REQUEST_LINE;
    headers_.clear();
    canneltimer();
    loop_->getepoll()->update(EPOLL_MOD(),channel_,DEFAULT_ALIVE_TIME);
}

void client::sendReplyToClient()
{
    auto sum = outbuffer_.size();
    size_t written = 0;
    size_t left = sum;

    while( left > 0 ){
        int cur_written = write(fd_,outbuffer_.c_str() + written,left);
        if( cur_written < 0 ){
            if( errno == EINTR )
                continue;
            else if( errno == EAGAIN ){
                break;
            }
        }
        written += cur_written;
        left -= cur_written;
    }

    outbuffer_.erase(0,written);
    if( !outbuffer_.empty() )
    {
        channel_->EnableWrite();
    }else if( channel_->GetEvents() & EPOLLOUT != 0)
    {
        channel_->DisableWrite();
    }    
}

void client::TimeoutHandle()
{
    state_ = ConnectionState::DISCONNECTED;
    loop_->getepoll()->update(EPOLL_DELETE(),channel_);
}

void client::regist()
{
    channel_->SetEvents( EPOLLIN | EPOLLET | EPOLLONESHOT );
    
    loop_->getepoll()->update(EPOLL_ADD(),channel_,DEFAULT_ALIVE_TIME);
    
}