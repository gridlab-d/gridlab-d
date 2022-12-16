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
// TODO: docstrings for cpp_threadpool
/**
 *
 */
//template<class function_sig=void()>
class cpp_threadpool {
private:
    std::vector<std::thread> Threads;
    std::mutex queue_lock, wait_lock, sync_mode_lock;
    std::atomic_int running_threads{0};
    std::atomic_bool exiting{false};
    std::condition_variable condition, sync_condition, wait_condition;
//    std::queue<std::function<function_sig>> job_queue;
    std::queue<std::function<void()>> job_queue;
    std::thread::id sync_id, shutdown_id;

    std::atomic_bool sync_mode{false}; // false for parallel mode, true for synchronous mode

    void sync_wait_on_queue();

    void wait_on_queue();

public:
    /**
     *
     */
    explicit cpp_threadpool(unsigned);

    /**
     *
     */
    ~cpp_threadpool();

    /**
     *
     * @param mode
     */
    inline void set_sync_mode(bool mode) { sync_mode.store(mode); }

    /**
     *
     * @param job
     * @return
     */
    bool add_job(std::function<void()> job);

    /**
    * Threadpool barrier. Blocks until all threads have completed all pending tasks and entered a ready state.
    */
    void await();

    void exit();

    /**
     *
     * @return
     */
    inline std::map<std::thread::id, int> get_threadmap() {
        int index = 0;
        std::map<std::thread::id, int> thread_map;
        for (auto &Thread: Threads) {
            thread_map.insert(std::pair<std::thread::id, int>(Thread.get_id(), index));
            index++;
        }
        return thread_map;
    }
};


#endif //_CPP_THREADPOOL_H
