// logger 类的实现

#include "logger.h"

#include "stdio.h"

using namespace summer;
using namespace summer::log;

void defaultoutput(char* msg, int len)
{
    int n = fwrite(msg,1,len,stdout);

    if ( n != len ){
        printf("fwrite error");
        abort();
    }
}

void defaultflush()
{
    fflush(stdout);
}

logger::logger( basename name, int line, Loglevel level)
    :level_(level),
    line_(line),
    stream_(),
    name_(name),
    outputfunc_(defaultoutput),
    flushfunc_(defaultflush){
        stream_ << "file:" << name.data() << ";" << "line:" << line << '\n' ;
    }


//将缓冲区中内容输出只文件流中
logger::~logger()
{
    stream_ << "finished" ;

    FixedBuffer<ksmallbuf>& buf(stream_.buffer());
    outputfunc_(buf.data(),buf.length());
}