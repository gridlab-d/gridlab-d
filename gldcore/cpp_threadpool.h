/*
 * cpp_threadpool.h
 *
 *  Created on: Aug 20, 2018
 *      Author: mark.eberlein@pnnl.gov
 *
 *      Provides standardized and safe C++ threadpool interface.
 */

#ifndef _CPP_THREADPOOL_H
#define _CPP_THREADPOOL_H


#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <chrono>
#include <functional>
#include <map>

using namespace std;

class cpp_threadpool {
private:
    vector<thread> Threads;
    mutex queue_lock, wait_lock, sync_mode_lock;
    atomic_int running_threads{0};
    atomic_bool exiting{false};
    condition_variable condition, wait_condition;
    queue<function<void()>> job_queue;

    atomic_bool sync_mode; // false for parallel mode, true for synchronous mode

    void wait_on_queue();

public:
    explicit cpp_threadpool(int);
    ~cpp_threadpool();
    inline void set_sync_mode(bool mode) { sync_mode.store(mode); }
    bool add_job(function<void()> job);
    void await();
    inline map<thread::id, int> get_threadmap(){
        int index = 0;
        map<thread::id, int> thread_map;
        for (auto &Thread : Threads) {
            thread_map.insert(std::pair<thread::id, int>(Thread.get_id(), index));
            index++;
        }
        return thread_map;
    }

};


#endif //_CPP_THREADPOOL_H
