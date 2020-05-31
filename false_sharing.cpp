/**
 *  Cache optimization example
 *  Based on cpp conference: https://www.youtube.com/watch?v=Nz9SiF0QVKY
 */

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

struct Point3D
{
    float x, y, z;
};

struct Color
{
    int r, g, b;
};
} ///< namespace common

namespace cache_non_friendly
{
class Shape
{
public:
    Shape()=default;
    ~Shape()=default;

    virtual void draw() = 0;
    virtual float calculate_area() = 0;

    typedef std::unique_ptr<Shape> Ptr;

protected:
    common::Color color;
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

    float calculate_area() override
    {
        // std::cout << "Circle::calculate_area" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        return 0.f;
    }
    typedef std::unique_ptr<Circle> Ptr;
private:
    common::Point3D center;
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

    float calculate_area() override
    {
        // std::cout << "Square::calculate_area" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return 0.f;
    }
    typedef std::unique_ptr<Square> Ptr;
private:
    common::Point3D top_left_pnt;
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
        float total_area = 0;
        for(const auto& shape : shapes)
        {
            total_area += shape->calculate_area();
        }
    }
}
} ///< namespace cache_non_friendly


namespace cache_friendly
{
/**
 *  What are we trying to do?
 *      - draw shapes
 *      - calculate area of shape
 *
 *  What do we need for that?
 *      - drawing: color and geometry of figure
 *      - area: geometry of figure
 */

struct CircleGeometry
{
    common::Point3D center;
    float radius;
};

struct SquareGeometry
{
    common::Point3D top_left_pnt;
    float side;
};

struct ShapesGeometry
{
    std::vector<CircleGeometry> circles;
    std::vector<SquareGeometry> squares;
};

void draw(const ShapesGeometry& geometry)
{

}

float calculate_area(const ShapesGeometry& geometry)
{
    common::ScopedTimerMs timer("area time");
    float total_area = 0.f;
    for(const auto& circle : geometry.circles)
    {
        // total_area += circle
    }

    for(const auto& square : geometry.circles)
    {
        // total_area += square
    }
    return total_area;
}

void benchmark_fn(const uint size)
{

}
} ///< namespace cache_friendly

int main()
{
    cache_non_friendly::benchmark_fn(1000);
    cache_friendly::benchmark_fn(1000);
    return 0;
}