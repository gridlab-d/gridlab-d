/*
 * cpp_threadpool.cpp
 *
 *  Created on: Aug 20, 2018
 *      Author: mark.eberlein@pnnl.gov
 *
 *      Provides standardized and safe C++ threadpool interface.
 */

#include "cpp_threadpool.h"

using namespace std;

cpp_threadpool::cpp_threadpool(int num_threads) {
    if (num_threads == 0) {
        num_threads = thread::hardware_concurrency();
    }

    sync_mode.store(false); // Default to parallel

    std::thread local_thread(&cpp_threadpool::sync_wait_on_queue, this);
    sync_id = local_thread.get_id();
    Threads.push_back(std::move(local_thread));

    for (int index = 0; index < num_threads; index++) {
        std::thread local_thread(&cpp_threadpool::wait_on_queue, this);
        Threads.push_back(std::move(local_thread));
    }
}

cpp_threadpool::~cpp_threadpool() {
    exiting.store(true); // = true;

    job_queue.push([]() {});


    for (auto &Thread : Threads) {
        job_queue.push([]() {});
        if(Thread.get_id() == sync_id)
            sync_condition.notify_one();
        else
            condition.notify_one();
    }

    for (auto &Thread : Threads) {
        if (Thread.joinable()) {
            Thread.join();
        }
    }
}

// This is a special single-threaded queue handler, for when sync mode is active.
void cpp_threadpool::sync_wait_on_queue() {

    function<void()> Job;
    unique_lock<mutex> lock(queue_lock);

    while (!exiting.load()) {
        if (!lock.owns_lock()) lock.lock();
        sync_condition.wait(lock,
                            [=] { return !job_queue.empty(); });

        if (job_queue.empty()) {
            continue;
        } else {
            Job = job_queue.front();
            job_queue.pop();
        }
        lock.unlock();
        Job();// function<void()> type

        wait_condition.notify_all();
    }
}

void cpp_threadpool::wait_on_queue() {

    function<void()> Job;
    unique_lock<mutex> lock(queue_lock);

    while (!exiting.load()) {
        if (!lock.owns_lock()) lock.lock();
        condition.wait(lock,
                       [=] { return !job_queue.empty(); });

        unique_lock<mutex> parallel_lock(sync_mode_lock);
        if (job_queue.empty()) {
            continue;
        } else {
            Job = job_queue.front();
            job_queue.pop();
        }
        lock.unlock();
        Job(); // function<void()> type

        parallel_lock.unlock();
        running_threads--;
        wait_condition.notify_all();
    }
}

bool cpp_threadpool::add_job(function<void()> job) {
    unique_lock<mutex> lock(queue_lock);
    try {
        running_threads++;
        job_queue.push(job);

        if (sync_mode.load()) {
            sync_condition.notify_one();
        } else {
            condition.notify_one();
        }
        return true;
    } catch (exception &) {
        return false;
    }
}

void cpp_threadpool::await() {
    unique_lock<mutex> lock(wait_lock);
    wait_condition.wait_for(lock, chrono::milliseconds(50), [=] { return running_threads.load() <= 0; });
}
