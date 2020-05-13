#include <thread>
#include <iostream>
#include <mutex>
#include <time.h>
class ThreadSafeData
{
private:
    struct{
        int x = 0;
        int y = 0;  
    } data;
    mutable std::mutex mtx_x;
    mutable std::mutex mtx_y;
public:
    ThreadSafeData() = default;
    int getX() const 
    { 
        std::lock_guard<std::mutex> lg(this->mtx_x);
        this->data.x; 
    }
    int getY() const 
    {
        std::lock_guard<std::mutex> lg(this->mtx_y); 
        this->data.y; 
    } 

    void setX(int x_) 
    {
        std::lock_guard<std::mutex> lg(this->mtx_x);
        data.x = x_;
    }
    
    void setY(int y_) 
    {
        std::lock_guard<std::mutex> lg(this->mtx_y); 
        data.y = y_;
    }

    int processData()
    {
        // Bad because you lock for the whole scope. Process Data might take long time
        std::lock_guard<std::mutex> lg_x(this->mtx_x);
        std::lock_guard<std::mutex> lg_y(this->mtx_y); 
        /// deadlock here
        int tmp_x = data.x;
        int tmp_y = data.y;

        return tmp_x * tmp_y;
    }

    int processData2()
    {
        std::unique_lock<std::mutex> lock_x(this->mtx_x, std::defer_lock);
        std::unique_lock<std::mutex> lock_y(this->mtx_y, std::defer_lock);
        std::lock(lock_x, lock_y);  ///<mutexes locked here

        int tmp_x = data.x;
        int tmp_y = data.y;

        lock_x.unlock();
        lock_y.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(2));
        return tmp_x * tmp_y;
    }
};

int main()
{
    ThreadSafeData ts_data;
    std::thread thread_update_x(&ThreadSafeData::setX, ts_data, 2);
    std::thread thread_update_y(&ThreadSafeData::setY, ts_data, 3);
    std::thread thread_process([&ts_data](){ std::cout << ts_data.processData() << std::endl;});
    return 0;
}