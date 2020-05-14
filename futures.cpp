#include <deque>
#include <mutex>
#include <future>
#include <thread>
#include <utility>
#include <iostream>
#include <functional>

int calculate() { return 42*42; }
void do_other_stuff() {}
int returning_value_async_foo()
{
    std::future<int> the_answer=std::async(calculate);
    do_other_stuff();
    std::cout<<"The answer is " << the_answer.get() << std::endl;
}


std::mutex m;
std::deque<std::packaged_task<void()>> tasks;

bool gui_shutdown_message_received() { return false; }
void get_and_process_user_input();

void gui_thread()
{
    while(!gui_shutdown_message_received())
    {
        get_and_process_user_input();
        std::packaged_task<void()> task;
        {
            std::lock_guard<std::mutex> lk(m);
            if(tasks.empty()) { continue; }
            task=std::move(tasks.front());
            tasks.pop_front();
        }
        task(); ///< when this finishes, future associated with that task will be ready
    }
}

template<typename Func>
std::future<void> post_task_for_gui_thread(Func f)
{
    std::packaged_task<void()> task(f);
    std::future<void> res=task.get_future();    ///< Here we get a future from a task

    std::lock_guard<std::mutex> lk(m);
    tasks.push_back(std::move(task)); ///< Add task to list shared with gui thread.
    return res;
}

int main()
{
    std::thread gui_bg_thread(gui_thread);
    
    std::function<void()> f_1( [](){std::cout << "task 1" << std::endl;} );
    std::future<void> f_1_result = post_task_for_gui_thread(f_1);
    
    auto f_2 = [](int a, int b){std::cout << "task 2" << std::endl;};
    std::future<void> f_2_result = post_task_for_gui_thread(std::bind(f_2, 1,2));   ///< C++14 feature - automatic template type deduction

    f_1_result.get();
    f_2_result.get();
}