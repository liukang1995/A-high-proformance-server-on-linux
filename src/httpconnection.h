// 一次http 链接，负责处理一个fd_上的请求
#include "eventloop.h"
#include "timer.h"

#include <memory>
#include <string>
#include <unordered_map>


namespace summer
{
    class httpconnection{

    public:
    // 
        enum class ProcessState;

        enum class URIState;

        enum class HeaderState;

        enum class AnalysisState;
    // 
        enum class ParseState;
    // TCP链接的状态
        enum class ConnectionState;
    // HTTP请求方法
        enum class HttpMethod;
    // mime类型
        enum class MineType;

        enum class HttpVision;

        httpconnection( eventloop* loop, int fd );
        ~httpconnection( );

        void settimer( std::shared_ptr<timernode> t );
        void canceltimer( );

        std::shared_ptr<channel> getchannel() { return channel_; }
        eventloop* getloop() { return loop_; }

        void handleclose();
        void newevent();

        void reset();
    private:
        
        void handleread();
        void handlewrite();
        void handleconnection();
        void handleerror(int fd, int errnum, std::string msg);

        URIState parseURI();
        HeaderState parseHeader();
        AnalysisState analysisRequest();

        void init_mime();
    // 文件描述符
        int fd_;

    // 处理当前请求的loop_,由构造函数指定
        eventloop* loop_;

    // 由httpconnection创建，事件分发类
        std::shared_ptr<channel> channel_;

    // 应用层输入输出缓冲区
        std::string inbuffer;
        std::string outbuffer;

        bool error_;

    // 当前连接状态
        ConnectionState connstate_;
        ProcessState proceState_;
        ParseState parseState_;

        HttpVision vision_;
        HttpMethod method_;

        std::string path_;
        std::string filename_;

        // 当前解析位置索引
        int index;

        bool keepalive_;

        std::map<std::string, std::string> headers_;
        std::weak_ptr<timernode> timer_;

        static int DEFAULT_EXPIRED_TIME;
        static int DEFAULT_ALIVE_TIME;

        std::unordered_map<std::string,std::string> mime_map;
    };// end httpconnection
} // end summer