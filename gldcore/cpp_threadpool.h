//
// Created by meberlein on 8/20/18.
//

#ifndef _CPP_THREADPOOL_H
#define _CPP_THREADPOOL_H


#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <chrono>

using namespace std;

class cpp_threadpool {
private:
    vector<thread> Threads;
    mutex queue_lock, wait_lock, sync_mode_lock;
    atomic_int running_threads{0};
    atomic_bool exiting{false};
    condition_variable condition, wait_condition;
    queue<function<void()>> job_queue;

    thread::id join_thread;

    atomic_bool sync_mode; // false for parallel mode, true for synchronous mode

    void wait_on_queue();

public:
    explicit cpp_threadpool(int);
    ~cpp_threadpool();
//    inline void set_sync_mode(bool mode){ std::call_once(setSyncModeFlag, [this, mode](){ sync_mode = mode; }); }
    inline void set_sync_mode(bool mode) { sync_mode.store(mode); }
    bool add_job(function<void()> job);
    void await();

};


#endif //_CPP_THREADPOOL_H
