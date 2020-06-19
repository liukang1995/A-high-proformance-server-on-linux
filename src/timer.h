// timernode: 一个计时器节点，它代表一个文件描述符和它关联的超时值
// timermanager: 管理timernode，取出超时节点，并执行超时任务
#include <time.h>

#include <functional>
#include <memory>
#include <queue>

namespace summer
{
    class HttpData;
    class timernode
    {
    public:
        typedef std::shared_ptr<HttpData> HttpDataPtr;

        timernode( HttpDataPtr data, time_t timeout );
        // timernode( const timernode& rhs );

        ~timernode();

        void update( time_t timeout );
        bool isValid();

        /*

        bool clearReq();

        */

       void setdeleted() { deleted = true; }
       bool isdeleted() { return deleted; }

       time_t getExpired() { return expiredTime; }

    private:
        bool deleted;
        time_t expiredTime;
        std::shared_ptr<HttpData> httpdata_;
    };


    class timerManager
    {
    public:
        timerManager() = default;
        ~timerManager() = default;

        typedef std::shared_ptr<timernode> TimeNodePtr;

        struct comp{
            bool operator()( TimeNodePtr& lhs, TimeNodePtr& rhs)
            {
                return lhs->getExpired() > rhs->getExpired();
            }
        };

        void addtimer(summer::timernode::HttpDataPtr data, time_t timeout);
        void handleExpired();

        static bool CompTimeNodePtr( TimeNodePtr& lhs, TimeNodePtr& rhs ) { return lhs->getExpired() > rhs->getExpired(); }

    private:
        

        std::priority_queue<TimeNodePtr, std::deque<TimeNodePtr>, comp > nodes;
    };

} // end summer