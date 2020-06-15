// 封装一个文件流，及其相关操作

#ifndef ROOT_SRC_LOG_FILE_H
#define ROOT_SRC_LOG_FILE_H

#include <sys/types.h>
#include <stdio.h>

namespace summer{
    namespace log{

class file
{
public:
    file(const char*);
    ~file();
    void append(char*,size_t);
    off_t writtenbytes(){ return writtenbytes_; }
    void flush(){ fflush(fp_); }

private:
    FILE* fp_;
    off_t writtenbytes_;
    char buf_[64 * 1024];
};

    }//end log
} // end summer
#endif