#include <iostream>
#include <list>
#include <stack>
#include <vector>
#include <random>
#include <algorithm>
#include <memory>
#include <thread>
#include <future>
#include <cassert>
struct empty_stack: public std::exception
{
    const char* what() const throw()
    {
        return "empty stack";
    }
};

template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack()=default;
    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m);
        data=other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    void push(const T& new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(new_value);
    }

    void push(T&& new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.emplace(std::move(new_value));
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) { return std::shared_ptr<T>(); }
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.top())));
        data.pop();
        return res;
    }

    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) { throw empty_stack(); }
        value=std::move(data.top());
        data.pop();
    }
    
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};


template<typename T>
struct sorter
{
    struct chunk_to_sort
    {
        std::list<T> data;
        std::promise<std::list<T>> promise;

        chunk_to_sort(){}
        /// std::promise is only movable 
        chunk_to_sort(const chunk_to_sort& other)=delete;
        chunk_to_sort(chunk_to_sort&& other) 
            : promise(std::move(other.promise)),
              data(std::move(other.data))
            {}
    };

    std::atomic<bool> end_of_data;
    unsigned const max_thread_count;
    std::vector<std::thread> threads;
    threadsafe_stack<chunk_to_sort> chunks;

    sorter():
        max_thread_count(std::thread::hardware_concurrency()-1),
        end_of_data(false)
    {}

    ~sorter()
    {
        end_of_data=true;
        for(unsigned i=0;i<threads.size();++i) { threads[i].join(); }
    }

    void try_sort_chunk()
    {
        std::shared_ptr<chunk_to_sort> chunk = chunks.pop();
        if(chunk) { sort_chunk(chunk); }
    }

    std::list<T> do_sort(std::list<T>& chunk_data)
    {
        if(chunk_data.empty()) { return chunk_data; }

        std::list<T> result;
        result.splice(result.begin(),chunk_data,chunk_data.begin());
        T const& partition_val=*result.begin();

        typename std::list<T>::iterator divide_point = std::partition(chunk_data.begin(),chunk_data.end(),
                                                           [&](T const& val){return val<partition_val;});
        chunk_to_sort new_lower_chunk;
        new_lower_chunk.data.splice(new_lower_chunk.data.end(), chunk_data,chunk_data.begin(),
                                    divide_point);

        std::future<std::list<T>> new_lower = new_lower_chunk.promise.get_future();
        chunks.push(std::move(new_lower_chunk));
        if(threads.size()<max_thread_count) { threads.push_back(std::thread(&sorter<T>::sort_thread,this)); }
        
        std::list<T> new_higher(do_sort(chunk_data));
        result.splice(result.end(),new_higher);
        while(new_lower.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            try_sort_chunk();
        }

        result.splice(result.begin(),new_lower.get());
        return result;
    }

    void sort_chunk(std::shared_ptr<chunk_to_sort > const& chunk)
    {
        chunk->promise.set_value(do_sort(chunk->data));
    }

    void sort_thread()
    {
        while(!end_of_data)
        {
            try_sort_chunk();
            std::this_thread::yield();
        }
    }
};

template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
    if(input.empty()) { return input; }
    sorter<T> s;
    return s.do_sort(input);
}

template<typename Numeric, typename Generator = std::mt19937>
Numeric generate_random_value(Numeric from, Numeric to)
{
    thread_local static Generator gen(std::random_device{}());

    using dist_type = typename std::conditional
    <
        std::is_integral<Numeric>::value
        , std::uniform_int_distribution<Numeric>
        , std::uniform_real_distribution<Numeric>
    >::type;

    thread_local static dist_type dist;

    return dist(gen, typename dist_type::param_type{from, to});
}

template<class T>
std::list<T> create_random_list(const size_t size, const T min, const T max)
{
    std::list<T> random_list;
    for(size_t i = 0; i < size; ++i)
    {
        T random_value = generate_random_value<T>(min, max);
        random_list.push_back(random_value);
    }
    return random_list;
}

template<typename T>
void print_data(const std::list<T>& data)
{
    std::cout << "Data: [";
    for(const T& element : data) { std::cout << element << ", "; }
    std::cout << "]" << std::endl << std::endl;
}

struct ScopedTimerMs
{
    typedef std::chrono::steady_clock::time_point time_point;
    const std::string name;
    const time_point begin;

    ScopedTimerMs(const std::string& name_) : 
        name(name_),
        begin(std::chrono::steady_clock::now())
    {}
    ~ScopedTimerMs()
    {
        const time_point end = std::chrono::steady_clock::now();
        std::cout << this->name << " time=" << 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - this->begin).count() << "[ms]" << std::endl;
    }
};

template<typename T, bool print = false>
void perform_test(const size_t size, const T min, const T max)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::list<T> test_data = create_random_list<T>(std::move(size), std::move(min), std::move(max));
    if(print) { print_data(test_data); }

    std::list<T> parallel_sorted_data;
    {
        ScopedTimerMs timer("parallel_sorted_data");
        parallel_sorted_data = parallel_quick_sort(test_data);
        if(print) { print_data(parallel_sorted_data); }
    }
    
    std::list<T> sequential_sorted_data(test_data); ///< prepare data
    {
        ScopedTimerMs timer("sequential_sorted_data");
        sequential_sorted_data.sort();
        if(print) { print_data(sequential_sorted_data); }
    }

    assert(parallel_sorted_data == sequential_sorted_data); ///< sorted data should be exactly the same
}

int main()
{
    // perform_test<int, true>(50, 0, 1000);
    // perform_test<int>(50000, 0, 1000);
    // perform_test<double>(50, 0.0, 1000.0);
    // perform_test<float>(50, 0.0f, 1000.0f);
    perform_test<int>(1000000, 0, 1000);
}