#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include <iostream>

/// queue holding data
namespace v1
{
template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue()
    {}

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        data_cond.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        value=std::move(data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return not data_queue.empty();});
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return false;
        value=std::move(data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};
} ///< namespace v1

/// queue holding shared pointers to data
namespace v2
{
template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<std::shared_ptr<T> > data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue(){}

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        value=std::move(*data_queue.front());
        data_queue.pop();
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
        {
            return false;
        }
        
        value=std::move(*data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        std::shared_ptr<T> res=data_queue.front();
        data_queue.pop();
        return res;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res=data_queue.front();
        data_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }

    void push(T new_value)
    {
        std::shared_ptr<T> data(
            std::make_shared<T>(std::move(new_value)));
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(data);
        data_cond.notify_one();
    }
};
}; ///< namespace v2

/// queue with fine grained locks
namespace v3
{

/// Implementation of queue container using linked list
template <typename T>
class queue
{
private:
    struct node
    {
        T data;
        std::unique_ptr<node> next;

        node() : next(nullptr) {}
        node(T data_) : data(std::move(data_)){}
    };
    std::unique_ptr<node> head;
    node* tail;

public:
    queue() : tail(nullptr) {}
    queue(const queue&) = delete;
    queue& operator=(const queue&) = delete;

    void push(T data)
    {
        std::unique_ptr<node> value_ptr(new node(std::move(data)));
        node* new_tail = value_ptr.get();
        
        if(this->tail) { this->tail->next = std::move(value_ptr); }
        else { this->head = std::move(value_ptr); }
        this->tail = new_tail;
    }
    std::shared_ptr<T> pop()
    {
        if(not this->head) { return std::shared_ptr<T>(); }
        const std::shared_ptr<T> head_value = std::make_shared<T>(this->head->data); ///< Access data from head

        const std::unique_ptr<node> old_head = std::move(this->head); ///< make temporary from current head
        this->head = std::move(old_head->next); ///< next is a new head
        if(not this->head) { this->tail = nullptr; }

        return head_value;
    }
    bool empty(){ if(not this->head){ return true; } else{ return false; }}
};
} ///< namespace v3

struct data_chunk{};
data_chunk prepare_data() { return data_chunk(); }
void prepare_data_thread(v1::threadsafe_queue<data_chunk>& rq)
{
    while(true)
    {
        const data_chunk data = prepare_data();
        rq.push(data);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void data_procesing_thread(v1::threadsafe_queue<data_chunk>& rq)
{
    while(true)
    {
        data_chunk data;
        rq.wait_and_pop(data);
    }
}

int main()
{
    // v1::threadsafe_queue<data_chunk> rq;
    // std::thread t_1(prepare_data_thread, std::ref(rq));
    // std::thread t_2(data_procesing_thread, std::ref(rq));

    v3::queue<int> queue_1;
    std::shared_ptr<int> val = queue_1.pop();
    if(not val) { std::cout << "empty" << std::endl; }
    queue_1.push(1);
    queue_1.push(2);
    val = queue_1.pop();
    std::cout << *(val.get()) << std::endl;
    val = queue_1.pop();
    std::cout << *(val.get()) << std::endl;
}