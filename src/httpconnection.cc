
#include "httpconnection.h"
#include "channel.h"
#include "log/logger.h"

#include "assert.h"
#include "stddef.h"
#include "stdlib.h"

#include "unistd.h"
#include "sys/epoll.h"
#include "sys/stat.h" 
#include "sys/mman.h"
#include "fcntl.h" // for open()

#include <regex>
#include <string>
#include <vector>
#include <future>

using namespace summer;
using namespace std;

namespace summer{

int httpconnection::DEFAULT_EXPIRED_TIME = 2000;
int httpconnection::DEFAULT_ALIVE_TIME = 5*60*1000;

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
                    LOGERROR << "read failed";
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

} // end summer

enum class httpconnection::ProcessState{
    START_PARSE_URI = 1,
    START_PARSE_HEADERS,
    START_RECV_BODY,
    START_ANALYSIS,
    STATE_FINISH
};

enum class httpconnection::HeaderState{
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum class httpconnection::URIState{
    PARSE_URI_SUCCESS = 1,
    PARSE_URI_AGAIN ,
    PARSE_URI_ERROR,
};

enum class httpconnection::AnalysisState{
    ANALYSIS_SUCCESS = 1,
    ANALYSIS_ERROR
};

enum class httpconnection::ParseState{
    H_START = 1,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

enum class httpconnection::ConnectionState{
    H_CONNECTED = 1,
    H_DISCONNECTING,
    H_DISCONNECTED
};

enum class httpconnection::HttpMethod{
    METHOD_POST = 1,
    METHOD_GET,
    METHOD_HEAD
};

enum class httpconnection::HttpVision{
    HTTP_10 = 1, 
    HTTP_11
};

enum class httpconnection::MineType{
    html,
    avi,
    bmp,
    c,
    doc,
    gif,
    gz,
    htm,
    ico,
    jpg,
    png,
    txt,
    mp3,
    def,
    sum
};

void httpconnection::init_mime()
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

httpconnection::httpconnection( eventloop* loop, int fd )
    :
    loop_( loop ),
    fd_( fd ),
    channel_( new channel(loop,fd) ),
    error_( false ),
    connstate_( ConnectionState::H_CONNECTED ),
    proceState_( ProcessState::START_PARSE_URI ),
    parseState_( ParseState::H_START ),
    vision_( HttpVision::HTTP_11 ),
    index( 0 ),
    keepalive_( false )
{
    init_mime();
    channel_->setreadcallback( bind(&httpconnection::handleread,this) );
    channel_->setwritecallback( bind(&httpconnection::handlewrite,this) );
    channel_->setclosecallback( bind(&httpconnection::handleconnection,this) );
}

httpconnection::~httpconnection()
{
    close(fd_);
}

//服务器出错，构建HTTP消息发送回客户端。
void httpconnection::handleerror(int fd, int errnum, std::string msg)
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
    write2fd(fd_,buf,strlen(buf));
    snprintf(buf,4096,"%s",body_buf.c_str());
    write2fd(fd_,buf,strlen(buf));
}

httpconnection::URIState httpconnection::parseURI()
{
    size_t pos = inbuffer.find("\r\n",index);

// 没有读到完整的一行
    if( pos == string::npos )
        return URIState::PARSE_URI_AGAIN;

    string request_line = inbuffer.substr(0,pos);
    // 清除已读数据
    //inbuffer.erase(0,pos+1);

    // if( (pos = request_line.find("GET")) != string::npos )
    // {
    //     method_ = HttpMethod::METHOD_GET;
    //     pos += 3;
    // }else if( (pos = request_line.find("POST")) != string::npos){
    //     method_ = HttpMethod::METHOD_POST;
    //     pos += 4;
    // }else if( (pos = request_line.find("HEAD") ) != string::npos){
    //     method_ = HttpMethod::METHOD_HEAD;
    //     pos += 4;
    // }else{
    //     return URIState::PARSE_URI_ERROR;
    // }
    
    // //assert( pos != request_line.size() );
    // pos += 1;
    // size_t e = request_line.find(' ',pos);
    // string request_uri = request_line.substr(pos, e-pos);
    // pos += (e-pos+1);
    
    // if( request_uri.empty() )
    //     perror("request error");
    
    // size_t f = request_uri.rfind('/');
    
    auto&& strs = split(request_line);

    index = pos + 2;
    if( strs.size() != 3 )
        perror( "split error" );

    if( strs[0] == "GET" ){
        method_ = HttpMethod::METHOD_GET;
    }else if( strs[0] == "POST" ){
        method_ = HttpMethod::METHOD_POST;
    }else if( strs[0] == "HEAD" ){
        method_ = HttpMethod::METHOD_HEAD;
    }else{
        return URIState::PARSE_URI_ERROR;
    }

    path_ = string("/home/lk/Desktop") + strs[1];
    auto& request_uri = strs[1];
    auto fb = request_uri.rfind('/');
    if( fb != request_uri.size() -1 )
        filename_ = request_uri.substr(fb+1);
    else
    {
        filename_ = "index.html";
    }

    if( strs[2] == "HTTP/1.1" ){
        vision_ = HttpVision::HTTP_11;
    }else{
        vision_ = HttpVision::HTTP_10;
    }

    return URIState::PARSE_URI_SUCCESS;
}

httpconnection::HeaderState httpconnection::parseHeader()
{
    auto p = index;
    while( true )
    {
        p = inbuffer.find("\r\n",index);
        if ( p == string::npos )
            return HeaderState::PARSE_HEADER_AGAIN;

        if ( p == index ){
            index += 2;
            return HeaderState::PARSE_HEADER_SUCCESS;
        }

        string head_line = inbuffer.substr(index,p-index);
        regex rgx( "([[:alpha:]]+-?[[:alpha:]]*):[[:blank:]]*(.*)" );
        smatch MatchResult;
        auto mth = regex_match(head_line,MatchResult,rgx);

        if( !mth )
            return HeaderState::PARSE_HEADER_ERROR;

        headers_[MatchResult.str(1)] = MatchResult.str(2);
        index = p + 2;
    }
}

httpconnection::AnalysisState httpconnection::analysisRequest()
{
    if( method_ == HttpMethod::METHOD_GET || method_ == HttpMethod::METHOD_HEAD )
    {
        string header;

        if(filename_ == "hello"){
            outbuffer += "HTTP/1.1 200 OK\r\nContent-Type:text/plain\r\n\r\nHello";
            return AnalysisState::ANALYSIS_SUCCESS;
        }

        int p = filename_.find('.');
        string file_type;
        if( p == string::npos ){
            file_type = mime_map["default"];
        }else{
            file_type = mime_map[filename_.substr(p)];
        }

        struct stat file_status;
        if( stat(path_.c_str(), &file_status) < 0 ){
            if( errno == ENOENT ){
                error_ = true;
                handleerror(fd_,404,"Not Found");
                inbuffer.clear();
                return AnalysisState::ANALYSIS_ERROR;
            }else{
                error_ = true;
                handleerror(fd_,400,"Bad Request");
                inbuffer.clear();
                return AnalysisState::ANALYSIS_ERROR;
            }
        }else{
            //存在此文件且可读
            header += "HTTP/1.1 200 OK";
            if ( headers_.find("Connection") != headers_.end() ){
                if(headers_["Connection"] == "Keep-Alive" || headers_["Connection"] == "keep-alive"){
                    keepalive_ = true;
                    header += "Connection: Keep-Alive\r\n";
                    header += "Keep-Alive: timeout=" + to_string(DEFAULT_ALIVE_TIME) +"\r\n";
                }
            }
            header += "Content-Type: " + file_type + "\r\n";
            header += "Content-Length: " + to_string(file_status.st_size) + "\r\n";
            header += "Server: Liukang's Web Server"; 
        }

        outbuffer += header;
        if( method_ == HttpMethod::METHOD_HEAD )
            return AnalysisState::ANALYSIS_SUCCESS;

        int source_fd = open(path_.c_str(),O_RDONLY);
        if( source_fd < 0 ){
            outbuffer.clear();
            handleerror(fd_, 404, "Not Found");
            return AnalysisState::ANALYSIS_ERROR;
        }
        void *mR = mmap(NULL,file_status.st_size,PROT_READ,MAP_PRIVATE,source_fd,0);
        close(source_fd);

        if( mR == MAP_FAILED )
        {
            munmap(mR,file_status.st_size);
            outbuffer.clear();
            handleerror(fd_,404,"Not Found!");
            return AnalysisState::ANALYSIS_ERROR;
        }

        outbuffer += string( static_cast<const char*>(mR),static_cast<const char*>(mR) + file_status.st_size );
        munmap(mR,file_status.st_size);
        return AnalysisState::ANALYSIS_SUCCESS;    
    }
}
void httpconnection::handlewrite()
{
    auto sum = outbuffer.size();
    size_t written = 0;
    size_t left = sum;

    while( left > 0 ){
        int cur_written = write(fd_,outbuffer.c_str() + written,left);
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

    outbuffer.erase(0,written);
    if( !outbuffer.empty() )
    {
        auto& event = channel_->getEvents();
        event |= EPOLLOUT;
    }
}

void httpconnection::handleconnection()
{
    // 清除计时器
    shared_ptr<timernode> t = timer_.lock();
    if( t )
    {
        t->clear();
        timer_.reset();
    }

    auto& event = channel_->getEvents();
    if( !error_ && connstate_ == ConnectionState::H_CONNECTED )
    {
        if( event != 0 )
        {
            int timeout = DEFAULT_EXPIRED_TIME;
            if( keepalive_ ) timeout = DEFAULT_ALIVE_TIME;

            if( (event & EPOLLIN) && (event & EPOLLOUT) ){
                event = 0;
                event |= EPOLLOUT;
            }
            event |= EPOLLET;
            loop_->update(channel_,timeout);
        }else if( keepalive_ ){
            event |= (EPOLLIN | EPOLLET);
            int timeout = DEFAULT_ALIVE_TIME;
            loop_->update(channel_,timeout);
        }else{
            event |= (EPOLLIN | EPOLLET);
            int timeout = (DEFAULT_ALIVE_TIME >> 1);
            loop_->update(channel_,timeout);
        }
    }else if( !error_ && connstate_ == ConnectionState::H_DISCONNECTING
            && (event & EPOLLOUT) ){
        event = ( EPOLLOUT | EPOLLET );
    }else{
        loop_->runInLoop( [this]{ handleclose();} );
    }
}

void httpconnection::handleread()
{
    auto& events = channel_->getEvents();

    //最后一次read返回0,
    bool zero = false;
    bool next = false;

    int readsum = read2string(fd_,inbuffer,zero);

    if ( readsum == -1 )
    {
        error_ = true;
        handleerror(fd_,400,"Server Error");
    }else{
        // 对端关闭写
        if(zero){
            connstate_ = ConnectionState::H_DISCONNECTING;
        }

        // 读到数据，进行解析
        if( proceState_ == ProcessState::START_PARSE_URI )
        {
            URIState flag = parseURI();
            if( flag == URIState::PARSE_URI_AGAIN ){
                
            }else if( flag == URIState::PARSE_URI_ERROR ){
                error_ = true;
                inbuffer.clear();
                handleerror(fd_,400,"Bad Request");
            }else{
                proceState_ = ProcessState::START_PARSE_HEADERS;
            }
        }

        if ( proceState_ == ProcessState::START_PARSE_HEADERS )
        {
            HeaderState flag = parseHeader();
            if( flag == HeaderState::PARSE_HEADER_AGAIN ){

            }else if( flag == HeaderState::PARSE_HEADER_ERROR ){
                error_ = true;
                inbuffer.clear();
                handleerror(fd_,400,"Bad Request");
            }else{
                if( method_ == HttpMethod::METHOD_POST ){
                    proceState_ = ProcessState::START_RECV_BODY;
                }else{
                    proceState_ = ProcessState::START_ANALYSIS;
                }
            }
        }

        if( proceState_ == ProcessState::START_RECV_BODY )
        {
            int content_length;
            if( headers_.find("Content-length") != headers_.end() ){
                content_length = stoi( headers_["Content-lenght"] );
                if( content_length == inbuffer.size() - index )
                    proceState_ = ProcessState::START_ANALYSIS;
            }else{
                error_ = true;
                inbuffer.clear();
                handleerror(fd_, 400, "Bad Request");
            }
        }

        if( proceState_ == ProcessState::START_ANALYSIS )
        {
            AnalysisState flag = analysisRequest();
            if( flag == AnalysisState::ANALYSIS_ERROR ){
                error_ = true;

                /*
                add 




                */
            }else{
                proceState_ = ProcessState::STATE_FINISH;
            }
        }
    }

    if( !error_ ){
        handlewrite();
    }

    if( !error_ && proceState_ == ProcessState::STATE_FINISH )
    {
        reset();
        if( inbuffer.size() > 0 && connstate_ != ConnectionState::H_DISCONNECTING ){
            handleread();
        }
    }else if( !error_ && connstate_ != ConnectionState::H_DISCONNECTED ){
        events |= EPOLLIN;
    }
}

void httpconnection::handleclose()
{
    connstate_ = ConnectionState::H_DISCONNECTED;
    loop_->remove(channel_);
}

void httpconnection::newevent()
{
    channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    loop_->add(channel_,DEFAULT_EXPIRED_TIME);
}