

#include "file.h"
#include "string.h"

using namespace summer::log;

file::file(const  char* name)
:fp_(fopen(name,"ae")),
writtenbytes_(0)
{
    setbuffer(fp_,buf_,sizeof(buf_));
}

file::~file()
{
    fclose(fp_);
}

void file::append(char* str,size_t len)
{
    size_t n = fwrite_unlocked(buf_,1,len,fp_);

    size_t remain = len - n;
    while ( remain > 0 ){

        size_t wz = fwrite_unlocked( buf_+n,1,remain,fp_ );

        if ( wz == 0 ) //没有向文件中写入任何内容，检查错误流
        {
            int errnum = ferror(fp_);
            if ( errnum )
            {
                char buf[512];
                fprintf(stderr, "FILE::append err: %s", strerror(errnum));

                break;
            }
        }

        n += wz;
        remain -= wz;
    }
    writtenbytes_ += n;
}

