#ifndef THREADEXCEPTIONHANDLE_H
#define THREADEXCEPTIONHANDLE_H

#include <iostream>
#include <string>
#include <unordered_map>
#include<vector>
#include <setjmp.h>
#include<signal.h>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
struct jmptagInfo{
  struct __jmp_buf_tag jmptag;
  uint addr;
  std::string name;
};
static boost::mutex mutex;
static std::unordered_map<uint, struct jmptagInfo> jmpMap;
static std::unordered_map<std::string, int> flagMap;
static struct __jmp_buf_tag sbuf;
static uint errStackBuff[1024];

class threadExceptionHandle
{
public:
    threadExceptionHandle();



    static void pushOne(std::string name, sigjmp_buf buf, uint threadID );

    static void clearOwnFlag(std::string name);
    static struct __jmp_buf_tag getJmpbuf(uint id);
    static void setAllFlag();

    static void setStackBuf(uint *buf);

    static void init();
};

#endif // THREADEXCEPTIONHANDLE_H
