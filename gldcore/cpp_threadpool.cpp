/*
 * cpp_threadpool.cpp
 *
 *  Created on: Aug 20, 2018
 *      Author: mark.eberlein@pnnl.gov
 *
 *      Provides standardized and safe C++ threadpool interface.
 */

#include "cpp_threadpool.h"

cpp_threadpool::cpp_threadpool(int num_threads) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
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

    sync_mode = true;
    add_job([]() {});

    sync_mode = false;

    for (auto &Thread : Threads) {
        if (Thread.get_id() != sync_id) {
            add_job([]() {});
        }
    }

    for (auto &Thread : Threads) {
        if (Thread.joinable()) {
            Thread.join();
        }
    }
}

// This is a special single-threaded queue handler, for when sync mode is active.
void cpp_threadpool::sync_wait_on_queue() {

    std::function<void()> Job;
    std::unique_lock<std::mutex> lock(queue_lock);

    while (!exiting.load()) {
        if (!lock.owns_lock()) lock.lock();
        sync_condition.wait(lock, [this] { return !job_queue.empty(); });

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

    std::function<void()> Job;
    std::unique_lock<std::mutex> lock(queue_lock);

    while (!exiting.load() && !(std::this_thread::get_id() == shutdown_id)) {
        if (!lock.owns_lock()) lock.lock();
        condition.wait(lock, [this] { return !job_queue.empty(); });

        std::unique_lock<std::mutex> parallel_lock(sync_mode_lock);
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

bool cpp_threadpool::add_job(std::function<void()> job) {
    std::unique_lock<std::mutex> lock(queue_lock);
    try {
        running_threads++;
        job_queue.push(job);

        if (sync_mode.load()) {
            sync_condition.notify_one();
        } else {
            condition.notify_one();
        }
        return true;
    } catch (std::exception &) {
        return false;
    }
}

void cpp_threadpool::await() {
    std::unique_lock<std::mutex> lock(wait_lock);
    wait_condition.wait(lock, [this] { return running_threads.load() <= 0; });
//    wait_condition.wait_for(lock, std::chrono::milliseconds(30), [this] { return running_threads.load() <= 0; });
}

#ifdef HAVE_GET_NPROCS
#include <sys/sysinfo.h>
int processor_count(void) { return get_nprocs(); }
#elif defined(__MACH__)
#include <sys/param.h>
#include <sys/sysctl.h>
int processor_count(void)
{
	int count;
	size_t size = sizeof(count);
	if (sysctlbyname("hw.ncpu", &count, &size, nullptr, 0))
		return 1;
	return count;
}
#else
int processor_count(void)
{
#ifdef _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#else
	char *proc_count = getenv("NUMBER_OF_PROCESSORS");
	int count = proc_count ? atoi(proc_count) : 0;
	return count ? count : 1;
#endif /* WIN32 */
}
#endif /* HAVE_GET_NPROCS */
