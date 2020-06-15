#include "logfile.h"

#include "unistd.h"

#include <cstdio>
#include <ctime>
#include <thread>
#include <string>
#include <iostream>

using namespace std;
using namespace summer::log;

logfile::logfile( string basename, off_t rollsize )
: basename_(basename),
    rollsize_(rollsize),
    mutex_(),
    last_roll(0),
    last_flush(0),
    startofperiod(0),
    count(0),
    CheckN_(1000)
{
    rollfile();
}

string logfile::getlogname()
{
// basename_2001-08-23_14:55:02_hostnametid.log

    string filename(basename_);

    time_t now = time(NULL);  
    
    struct  tm t;
    gmtime_r( &now, &t );

//将时间格式化输出只buf，2001-08-23_14:55:02
    char buf[32];
    strftime( buf, sizeof(buf), "_%F_%T_", &t );

    filename += buf;

// 添加主机名
    char hname[32];
    gethostname( hname, sizeof(hname) );
    filename += hname;

// 添加线程号
    std::thread::id thread_id = this_thread::get_id();

    std::hash<thread::id> hasher;
    size_t tid = hasher(thread_id);
    char tid_[12];
    snprintf( tid_, sizeof(tid), "%zu", tid );

    filename += tid_;

// 添加后缀
    filename += ".log";

    return filename;
}

void logfile::append( char* logline, size_t len )
{
    lock_guard<mutex> lck(mutex_);
// 向文件中写入len个字节数据
    file_->append( logline, len );

// 增加当前写入次数统计
    count++;

    if( file_->writtenbytes() > rollsize_) // 当前文件写满
    {
        rollfile();
    }else if ( count > CheckN_ ) // 每CheckN_ 次进行24小时滚动，flush检查
    {
        count = 0;
        time_t now = time(NULL);
        time_t cur_day = (now/86400) * 86400; 

        if ( cur_day != startofperiod ){
            rollfile();
        }

        if ( now - last_flush >  3 ){

            last_flush = now;
            flush();
        }

    }

}

void logfile::flush(){

    lock_guard<mutex> lck(mutex_);
    file_->flush();

}

void logfile::rollfile()
{
    time_t now = time(NULL);

    lock_guard<mutex> lck( mutex_ );

    last_flush = now;
    last_flush = now;
    startofperiod = now/86400 * 86400;

    string name = getlogname();
    file_.reset( new file( name.c_str() ));
}