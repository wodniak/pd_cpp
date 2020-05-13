#include <deque>
#include <cstddef>
#include <iostream>
#include <stack>

#include <exception>
#include <memory>
#include <thread>

/*
template<typename T,typename Container=std::deque<T> >
class stack
{
public:
    explicit stack(const Container&);
    explicit stack(Container&& = Container());
    ...
    bool empty() const;
    size_t size() const;
    T& top();
    T const& top() const;
    void push(T const&);
    void push(T&&);
    void pop();
    void swap(stack&&);
};
*/

/* 
For single-threded code this code is valid, for multi-threaded no. There might be a call to pop()
from another thread that removes the last element in between the call to empty() and the call to top()
*/
void do_sth(const int value) { std::cout << value; };
void example_on_std_stack()
{
    std::stack<int> s;
    s.push(1);
    auto f_1 = [](std::stack<int>& s){
        if(!s.empty())
        {
            const int value = s.top();
            s.pop();
            do_sth(value);
        }
    };

    /// Ordering threads here doesnt guarantee that t_1 will execute its function first.
    std::thread t_1(f_1, std::ref(s));
    std::thread t_2([](std::stack<int>& s){ s.pop(); }, std::ref(s));
    
    t_1.join();
    t_2.join();
}


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
    threadsafe_stack(){}
    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m);
        data=other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(new_value);
    }
    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
        data.pop();
        return res;
    }
    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if(data.empty()) throw empty_stack();
        value=data.top();
        data.pop();
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

int main()
{
    example_on_std_stack();

    threadsafe_stack<int> si;
    si.push(5);
    si.pop();
    if(!si.empty())
    {
        int x;
        si.pop(x);
    }
}