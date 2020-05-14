#include <thread>
#include <numeric>
#include <algorithm>
#include <functional>
#include <vector>
#include <iostream>
#include <chrono>
#include <future>

template<typename Iterator,typename T>
struct accumulate_block
{
    void operator()(Iterator first,Iterator last,T& result)
    {
        result=std::accumulate(first,last,result);
    }
};

template<typename Iterator,typename T>
struct accumulate_block_for_async
{
    int operator()(Iterator first,Iterator last)
    {
        int result;
        std::accumulate(first,last,result);
        return result;
    }
};

template<typename Iterator,typename T>
T parallel_accumulate(Iterator first,Iterator last,T init)
{
    unsigned long const length = std::distance(first,last);
    if(!length){ return init; }

    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread-1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads:2, max_threads);
    unsigned long const block_size = length / num_threads;
    std::cout << "Hardware threads: " << hardware_threads << "  Started threads: " << num_threads << std::endl; 

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads-1);

    Iterator block_start=first;
    for(unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end=block_start;
        std::advance(block_end,block_size);
        threads[i] = std::thread(accumulate_block<Iterator,T>(), block_start, block_end, std::ref(results[i]));
        block_start=block_end;
    }
    accumulate_block<Iterator,T>()(block_start,last,results[num_threads-1]);
    std::for_each(threads.begin(),threads.end(), std::mem_fn(&std::thread::join));
    return std::accumulate(results.begin(),results.end(),init);
}

template<typename Iterator,typename T>
T parallel_accumulate_async(Iterator first,Iterator last,T init)
{
    unsigned long const length = std::distance(first,last);
    if(!length){ return init; }

    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread-1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads:2, max_threads);
    unsigned long const block_size = length / num_threads;
    std::cout << "Hardware threads: " << hardware_threads << "  Started threads: " << num_threads << std::endl; 

    std::vector<std::future<uint>> futures;
    futures.reserve(num_threads-1);

    /// start tasks
    Iterator block_start=first;
    for(unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end=block_start;
        std::advance(block_end,block_size);
        futures[i] = std::async(std::launch::async, accumulate_block_for_async<Iterator,T>(), block_start, block_end);
        block_start=block_end;
    }
    init += accumulate_block_for_async<Iterator,T>()(block_start,last);  ///< start last block 
    for(auto& fut : futures){ init += fut.get(); } ///< gather data from all tasks
    return init;
}


int main()
{
    const uint number_of_points = 100'000'000; 
    std::vector<int> vi;
    vi.reserve(number_of_points);;
    for(int i=0; i < number_of_points;++i) { vi.emplace_back(3); }

    /// with thread objects
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    int sum_parallel = parallel_accumulate(vi.begin(),vi.end(), 0);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "sum_parallel = " << sum_parallel << "  time=" <<  std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count() << "[ns]" << std::endl;

    /// with asyncs and futures
    begin = std::chrono::steady_clock::now();
    int sum_parallel_async = parallel_accumulate(vi.begin(),vi.end(), 0);
    end = std::chrono::steady_clock::now();
    std::cout << "sum_parallel_async = " << sum_parallel_async << "  time=" <<  std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count() << "[ns]" << std::endl;

    /// Single threaded
    int sum = 0;
    begin = std::chrono::steady_clock::now();
    sum = std::accumulate(vi.begin(),vi.end(), sum);
    end = std::chrono::steady_clock::now();
    std::cout << "sum_single_threaded = " << sum << "  time=" <<  std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count() << "[ns]" << std::endl;
}
