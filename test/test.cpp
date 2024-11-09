#include "../code/log/log.h"
#include "../code/pool/threadpool.h"
#include <features.h>

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

void TestLog(){
    int cnt = 0;
    Log::Instance() -> init(0, "./testlogSync",".log",0);
    for(int level = 3;level>=0;level--){
        Log::Instance()->SetLevel(level);
        for(int i = 0;i<10000;i++){
            for(int j = 0;j<4;j++){
                LOG_BASE(j,"%s ---------- %d ==========", "Test", cnt++);
            }
        }
    }

    cnt = 0;
    Log::Instance()->init(0, "testlogASync", ".log", 5000);
    for(int level = 0;level<4;level++){
        Log::Instance()->SetLevel(level);
        for(int i = 0;i<10000;i++){
            for(int j = 0;j<4;j++){
                LOG_BASE(j, "%s ++++++++++ %d ==========", "Test", cnt++);
            }
        }
    }
}

void ThreadLogTask(int i,int cnt){
    for(int j = 0;j<10000;j++){
        LOG_BASE(i, "PID:[%04d]======= %05d ========= ", gettid(), cnt++);
    }
}

void TestThreadPool() {
    Log::Instance()->init(0,"./testThreadPool",".log",5000);
    ThreadPool threadpool(6);
    for(int i = 0;i<18;i++){
        threadpool.AddTask(std::bind(ThreadLogTask,i%4,i*10000));
    }
    getchar();
}

int main(){
    TestLog();
    //TestThreadPool();
}