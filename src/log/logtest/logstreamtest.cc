#include "../logstream.h"

using namespace summer::log;

int main()
{
    logstream log;
    log << 2;
    log << "hello";
    log << true;
    log << 2.0;
    return 0;
}