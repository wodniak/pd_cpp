#include <iostream>
#include <thread>
class A
{
private:
    int x = 2;

public:
    A() = default;
    void foo() { std::cout << this->x; }
};

class B
{
public:
    template<typename _Callable, typename... _Args>
    explicit B(_Callable&& __f, _Args&&... __args)
    {
        std::bind(std::forward<_Callable>(__f), std::forward<_Args>(__args)...);
    }
};

int main()
{
    A a;
    B b(&A::foo, &a);
    return 0;
}