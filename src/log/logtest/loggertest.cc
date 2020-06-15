#include "../logger.h"

using namespace summer::log;

int main()
{
    logger log( "file" , 10, WARN );
    logstream& a = log.stream();
    a << 2;
    a << "hello";
    log.stream() << "hello";

    logger( "file" , 10, WARN ).stream() << "hello" << 3;
    logger( "file" , 10, WARN ).stream() << 2;
    
    #define stm logger( "file" , 10, WARN ).stream()
    stm << 2;
}