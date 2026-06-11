# Plan for Improving Multithreading Efficiency in [`gldcore/exec.cpp`](gldcore/exec.cpp)

## Summary
This plan provides actionable steps for improving multithreading efficiency in [`gldcore/exec.cpp`,`gldcore/loadshape.cpp`,`gldcore/enduse.cpp`,`gldcore/schedule.cpp`](...), ensuring deterministic, compatible, and portable optimization for typical desktop hardware using `std::thread`. 

The single threaded code should remain unchanged for direct comparison. 
Any existing multithreading code may be changed or removed to maximize performance. 
Special attention should be given to syncing objects with the same parent in the same thread or using parent-level locks to prevent conflicts. 

Benchmarking and validation should use [`test_parallel/bench_500_houses.json`](test_parallel/bench_500_houses.json) as a representative workload. All test runs must be error-free, including convergence and other failures. 

Thread count for multithreading is controlled by `--threadcount []`, [] replaced by number used. 

## 1. Summary of Current Implementation
- Multithreading uses a thread pool and groups objects by parent to minimize lock contention.
- Threads are launched only for non-empty groups, synchronized with a latch and atomic flag.
- Bottlenecks: parent-lock contention, thread pool overhead, synchronization costs, workload imbalance.
- Singlethreaded method processes objects sequentially, avoiding thread overhead but not leveraging parallelism.

## 2. Constraints
- Multithreaded code must be deterministic: results must match singlethreaded mode for identical input.
- Backward compatibility: multithreaded and singlethreaded modes must produce equivalent results.
- **Leave the current singlethreading code untouched for comparison and benchmarking.**
- **Any existing multithreading code may be changed or removed as needed to achieve performance improvements.**
- **Questions can be asked along the way if clarification or additional information is needed.**
- **Explicitly ignore any and all code concerning `global_threadcount`. Do not change or interact with this value for object syncing.**

## 3. Target Hardware & Library
- Optimize for typical desktop (4-8 cores).
- Use C++ `std::thread` for portability and maintainability.

## 4. Multithreading Focus Instructions

### 4.1 Scope
- **All multithreading improvements must focus on the section of [`gldcore/exec.cpp`](gldcore/exec.cpp) where `ss_do_object_sync` is called.**
- **If changes are needed to object syncing logic, update [`gldcore/object.cpp`](gldcore/object.cpp) and [`gldcore/object.h`](gldcore/object.h) as required.**
- **Do NOT make unrelated changes elsewhere in the codebase. Stay strictly within the multithreading region and related object sync logic.**

### 4.2 Profiling & Benchmarking
- Profile both singlethreaded and multithreaded regions to identify hotspots.
- Benchmark with representative workloads to measure speedup and verify determinism.
- **Use [`test_parallel/bench_500_houses.json`](test_parallel/bench_500_houses.json) as a benchmark workload for performance and correctness validation.**
- **Ignore the "Parallelism" value in the profile output, as it is tied to `global_threadcount` and not `altThreadcount`.**

### 4.3 Workload Partitioning
- Ensure even distribution of objects across threads to avoid imbalance.
- Refine grouping logic to minimize lock contention and maximize parallelism.
- **Objects with the same parent should be synced in the same thread to prevent conflict. Alternatively, implement locks on parent resources if that provides better performance or safety.**

### 4.4 Synchronization
- Use minimal synchronization primitives (prefer atomic operations, avoid unnecessary mutexes).
- Ensure thread-safe access to shared data, but avoid over-synchronization.
- **Evaluate whether parent-level locking or thread assignment is optimal for preventing conflicts.**

### 4.5 Determinism Enforcement
- Enforce deterministic ordering of operations (e.g., fixed thread assignment, ordered reduction of results).
- Use thread barriers/latches to synchronize completion.

### 4.6 Error Handling
- Ensure exceptions and invalid states are handled consistently across threads.
- Propagate errors to main thread for unified handling.

### 4.7 Testing & Validation  - TODO
- Develop unit and integration tests comparing singlethreaded and multithreaded outputs.
- Use randomized and edge-case workloads to validate correctness and determinism.
- **Benchmark and validate using [`test_parallel/bench_500_houses.json`](test_parallel/bench_500_houses.json).**
- **Any error during the test run, including convergence errors or other failures, is not allowed and must be resolved.**

### 4.8 Documentation
- Document thread model, synchronization strategy, and determinism guarantees.
- Provide usage guidelines for maintainers and contributors.
- **Update this document as needed to provide clearer instructions based on each iteration and observed results.**

## 5. Related Code & Integration
- Review and update related headers (e.g., [`gldcore/exec.h`](gldcore/exec.h)) and utilities as needed.
- Ensure compatibility with existing build and test infrastructure.
- Ignore any existing `cpp_threadpool` files or code unless explicitly instructed otherwise, as they may be outdated or not relevant to the current multithreading implementation.
- Use a single persistent thread pool instance for the entire simulation run to avoid thread creation/destruction overhead and prevent deadlocks or resource exhaustion.

## 6. Build Details
- To update the executable after code changes, run the following command in the `/build` folder:

  ```sh
  cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . -j8
  ```

## 7. Thread Count Configuration
- If you need to change the thread count for multithreading, update the value of `--threadcount 8` on the commandline.
- To validate using single thread `--validate`. starts a single threaded test case loop running a case with a single thread. 
- To validate using multiple cases `--threadcount 8 --validate`, starts multiple test cases(threads) with a single thread. 
- To validate using a case multiple threads `--validate --threadcount 8`, starts a single threaded test case loop running a case with multiple threads. 
- TODO - Implement an option to run validate with multiple test cases with multiple threads 

## 8. Performance Expectations
- Current benchmark for the release branch using singlethreaded code is **~20 seconds** to complete.
- Current benchmark for the release branch using multithreaded code with 3 threads is **~28 seconds** to complete.
- **Success criteria:** Multithreaded execution must be **at least 20% faster** than singlethreaded execution (i.e., must complete in ≤16 seconds for the benchmark workload).
- **Continue to iterate on this task until multithreading is faster than singlethreading.**

---


