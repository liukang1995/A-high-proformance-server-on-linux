// 

#include "logstream.h"

#include <algorithm>

#include <stdio.h>
#include <string.h>

using namespace std;
using namespace summer;
using namespace summer::log;

namespace summer
{
    
    namespace log{

        template <class T>
        int convert( char buf[], T value )
        {
            static const char digit[] = "9876543210123456789";
            static const char* zero = digit+9;

            T i = value;
            char *p = buf;

            do{
                int idx = static_cast<int>( i % 10 );
                i /= 10;
                *p++ = zero[idx];
            }while( i != 0)

            if ( value < 0 ){
                *p++ = '-';
            }
            *p++ = '\0';

            reverse(buf,p);
            return p-buf;
        }
    } // namespace log
    
} // namespace summer

template<class T>
void logstream::converttobuf(T val){

    char* cur = buf_.current();
    int avail = static_cast<int>( buf_.avail() );

    if ( avail > num_reverse ){
        int len = convert( cur, val );
        buf_.add(len);
    }
}

logstream& logstream::operator<<(int value)
{
    converttobuf(value);

    return *this;
}

logstream& logstream::operator<<(short value)
{
    converttobuf(value);

    return *this;
}

logstream& logstream::operator<<(unsigned int value)
{
    converttobuf(value);

    return *this;
}

logstream& logstream::operator<<( unsigned short value )
{
    converttobuf(value);

    return *this;
}

logstream& logstream::operator<<( double value )
{
    char* cur = buf_.current();
    int avail = buf_.avail();

    if ( avail > num_reverse ){
        int len = snprintf( cur, num_reverse, "%.12g",value );
        buf_.add(len);
    }
    return *this;
}

logstream& logstream::operator<<( float value )
{
    operator<<( static_cast<double>(value) );
    return *this;
}

logstream& logstream::operator<<( char c )
{
    buf_.append(&c, 1);
    return *this;
}

logstream& logstream::operator<<( char* str )
{
    if ( str ){
        buf_.append(str, strlen(str));
    }
    return *this;
}

logstream& logstream::operator<<( string s)
{
    if ( buf_.avail() > s.size() ){
        buf_.append(s.c_str(), s.size() );
    }
    return *this;
}

