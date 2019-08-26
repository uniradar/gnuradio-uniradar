#include "threadexceptionhandle.h"
#include <setjmp.h>
#include<signal.h>
#include<threadexceptionhandle.h>
#include <execinfo.h>

void recvSignal(int sig)
{
    printf("received signal %d !!!\n",sig);
    fflush(0);
    uint id = pthread_self();
    jmp_buf env;

    env[0] = threadExceptionHandle::getJmpbuf(id);
    siglongjmp(env,id);
}
threadExceptionHandle::threadExceptionHandle()
{

}

void threadExceptionHandle::pushOne(std::string name, sigjmp_buf buf,uint threadID)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    struct jmptagInfo info;
    info.jmptag = buf[0];
    info.addr = threadID;
    info.name = name;
    jmpMap[threadID] = info;
    flagMap[name] = 0;
    sbuf = buf[0];
}

void threadExceptionHandle::clearOwnFlag(std::string name)
{
    std::cout <<name<< " clearOwnFlag~+++~\n" ;
    boost::unique_lock<boost::mutex> lock(mutex);
    flagMap[name] = 0;
}
void threadExceptionHandle::setAllFlag()
{
    boost::unique_lock<boost::mutex> lock(mutex);
    for (const auto& n : flagMap) {
//            std::cout << "Key:[" << n.first << "] Value:[" << n.second << "]\n";
           flagMap[n.first] = 1;
    }
}

void threadExceptionHandle::setStackBuf(uint *buf)
{
    for(int i=0;i<1024;i++)
        errStackBuff[i] = buf[i];
}

void threadExceptionHandle::init()
{
    printf("threadExceptionHandle::init(),reg: SIGSEGV|SIGFPE|SIGSYS|SIGBUS");
    fflush(0);
    signal(SIGSEGV,recvSignal);//合法地址的非法使用，产生段错误
    signal(SIGFPE, recvSignal);//非法数学运算，比如除0
    signal(SIGSYS, recvSignal);//非法的系统调用
    signal(SIGBUS, recvSignal);//非法地址访问
}
__jmp_buf_tag threadExceptionHandle::getJmpbuf(uint id)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    std::cout << "threadExceptionHandle::getJmpbuf~~~~~~~~~~~~~~~~~~~\n" ;
    return jmpMap[id].jmptag;
//    for (const auto& n : nameMap) {
//            if(n.second.addr == id)
//             {
//                std::cout << "return  ~~~~~~~~"<<n.first ;
//                return nameMap[n.first].jmptag;
//            }
//        }
//    std::cout << "return  sbufn" ;
    return sbuf;
}
