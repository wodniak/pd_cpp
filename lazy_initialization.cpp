#include <iostream>
#include <mutex>
#include <memory>
#include <thread>
///single-threaded code
/// @{

struct heavy_resource
{
    heavy_resource(){ std::this_thread::sleep_for(std::chrono::seconds(2)); }
    void do_something(){ std::this_thread::sleep_for(std::chrono::milliseconds(2)); }
};
std::shared_ptr<heavy_resource> resource_ptr;

void single_thread_foo() 
{
    if(not resource_ptr) { resource_ptr.reset(new heavy_resource()); }
    resource_ptr->do_something();
}
/// @}

///thread-safe lazy initialization with mutex - unnecesary serialization
/// @{
std::shared_ptr<heavy_resource> resource_ptr;
std::mutex resource_mutex;
void thread_safe_with_mutex_foo()
{
    std::unique_lock<std::mutex> lk(resource_mutex);
    if(!resource_ptr) { resource_ptr.reset(new heavy_resource); }
    lk.unlock();
    resource_ptr->do_something();
}
/// @}

///thread-safe lazy initialization with mutex - double-checked locking pattern - bad idea
/// @{
std::shared_ptr<heavy_resource> resource_ptr;
std::mutex resource_mutex;
void double_checked_locking_foo()
{
    if(!resource_ptr) ///<this read is not in sync with write done by another thread inside the lock
    {
        std::unique_lock<std::mutex> lk(resource_mutex);
        if(!resource_ptr) { resource_ptr.reset(new heavy_resource); }
        lk.unlock();
    }
    resource_ptr->do_something();
}
/// @}

///thread-safe lazy initialization without mutex
/// @{
std::shared_ptr<heavy_resource> resource_ptr;
std::once_flag resource_flag;
void thread_safe_without_mutex_foo()
{
    auto init_resource = [&](){resource_ptr.reset(new heavy_resource); };
    std::call_once(resource_flag, init_resource);
    resource_ptr->do_something();
}
/// @}


/// Bigger example
/// @{
#include <mutex>

struct connection_info{};
struct data_packet{};
struct connection_handle
{
    void send_data(data_packet const&){}
    data_packet receive_data(){ return data_packet(); }
};
struct remote_connection_manager
{
    connection_handle open(connection_info const&)
    {
        return connection_handle();
    }
};

class DatabaseHandler
{
private:
    connection_info connection_details;
    connection_handle connection;
    remote_connection_manager connection_manager;
    std::once_flag connection_init_flag;

    void open_connection()
    {
        connection=connection_manager.open(connection_details);
    }
public:
    DatabaseHandler(connection_info const& connection_details_):
        connection_details(connection_details_)
    {}
    void send_data(data_packet const& data)
    {
        std::call_once(connection_init_flag,&DatabaseHandler::open_connection,this);
        connection.send_data(data);
    }
    data_packet receive_data()
    {
        std::call_once(connection_init_flag,&DatabaseHandler::open_connection,this);
        return connection.receive_data();
    }
};
/// @}

int main()
{}