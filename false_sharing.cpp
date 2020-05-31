#include<iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>

namespace common
{
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
} ///< namespace common

namespace cache_non_friendly
{
struct Point3D
{
    float x, y, z;
};

struct Color
{
    int r, g, b;
};

class Shape
{
public:
    Shape()=default;
    ~Shape()=default;

    virtual void draw() = 0;
    virtual void calculate_area() = 0;

    typedef std::unique_ptr<Shape> Ptr;

protected:
    Color color;
    bool is_visible;
};

class Circle : public Shape
{
public:
    Circle()=default;
    ~Circle()=default;

    void draw() override
    {
        // std::cout << "Circle::draw" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    void calculate_area() override
    {
        // std::cout << "Circle::calculate_area" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    typedef std::unique_ptr<Circle> Ptr;
private:
    Point3D center;
    float radius;
};

class Square : public Shape
{
public:
    Square()=default;
    ~Square()=default;

    void draw() override
    {
        // std::cout << "Square::draw" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void calculate_area() override
    {
        // std::cout << "Square::calculate_area" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    typedef std::unique_ptr<Square> Ptr;
private:
    Point3D top_left_pnt;
    float side;
};

void benchmark_fn(const uint size)
{
    using ShapePtr = Shape::Ptr;
    std::vector<ShapePtr> shapes;
    shapes.reserve(size);
    for(uint i = 0; i < size; ++i)
    {
        ShapePtr shape;
        if(i % 2 == 0) { shape = std::make_unique<Square>(); }
        else { shape = std::make_unique<Circle>(); }
        shapes.push_back(std::move(shape));
    }

    /// Measure time for each call
    {
        common::ScopedTimerMs timer("draw time");
        for(const auto& shape : shapes)
        {
            shape->draw();
        }
    }

    {
        common::ScopedTimerMs timer("area time");
        for(const auto& shape : shapes)
        {
            shape->calculate_area();
        }
    }
}
} ///< namespace cache_non_friendly


int main()
{
    cache_non_friendly::benchmark_fn(1000);
    return 0;
}