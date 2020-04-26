#include <mutex>
#include <functional>
class some_data
{
    int a;
    std::string b;
public:
    void do_something()
    {}
};

class data_wrapper
{
private:
    some_data data;
    std::mutex m;
public:
    template<typename Function>
    void process_data(Function func)
    {
        std::lock_guard<std::mutex> l(m);
        func(data);
    }
};


class SingleThreadedClass
{
private:
    some_data* unprotected;

public:
    void do_something_with_data(some_data& protected_data)
    {
        unprotected=&protected_data;
    }

    void do_something_else()
    {
        unprotected->do_something();
    }
};

int main()
{
    data_wrapper x;
    SingleThreadedClass y;
    auto malicious_function = std::bind(&SingleThreadedClass::do_something_with_data, &y, std::placeholders::_1);
    x.process_data(malicious_function);
    y.do_something_else();
}
