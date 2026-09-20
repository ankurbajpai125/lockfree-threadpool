# lockfree-threadpool

A small C++17 concurrency library: a thread pool and a lock-free
single-producer/single-consumer (SPSC) ring buffer, with tests, sanitizer
builds, and a benchmark against a mutex-based queue.

## Components

- `tp::ThreadPool` - fixed worker threads, mutex + condition variable queue.
  `submit()` accepts any callable plus arguments and returns a `std::future`,
  so results and exceptions propagate back to the caller. The destructor
  drains pending tasks and joins all workers.
- `tp::SpscQueue<T>` - lock-free bounded ring buffer. One thread may push and
  one thread may pop. Uses `std::atomic` indices with acquire/release ordering,
  power-of-two capacity (bitmask indexing), and cache-line-aligned head/tail
  to avoid false sharing.
- `tp::MutexQueue<T>` - mutex-protected bounded queue with the same interface,
  used as the benchmark baseline.

## Build and test

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    ctest --test-dir build --output-on-failure

Sanitizer builds (Linux, GCC/Clang):

    cmake -S . -B build-tsan -G Ninja -DENABLE_TSAN=ON -DCMAKE_BUILD_TYPE=Debug
    cmake -S . -B build-asan -G Ninja -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug

## Benchmark

One producer thread, one consumer thread, 1,000,000 items per run, capacity
1024, median of 5 runs, Release build. Measured on WSL2 Ubuntu, GCC 15.2,
Intel Core i5-1035G1, 8 logical cores.

| Queue          | Throughput        |
|----------------|-------------------|
| Mutex queue    | 1.06 M items/sec  |
| Lock-free SPSC | 12.80 M items/sec |

Run it yourself:

    cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release
    ./build-release/benchmarks/queue_bench

## Design notes

- The SPSC queue needs no lock because each index has exactly one writer:
  only the producer writes `tail`, only the consumer writes `head`.
- The producer publishes with a release-store on `tail`; the consumer reads it
  with an acquire-load, which guarantees it sees the element written before.
- Task execution happens outside the pool's lock, so tasks run in parallel.
- Correctness is checked with GoogleTest under ThreadSanitizer and
  AddressSanitizer/UBSan.

## Limitations

- The SPSC queue is unsafe with more than one producer or consumer.
- The thread pool itself uses a mutex-based queue (many producers and
  consumers); it is not lock-free.
- The benchmark measures one scenario (1 producer, 1 consumer, spin-waiting)
  on one machine; results will vary with hardware and workload.
