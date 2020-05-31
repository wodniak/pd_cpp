/**
 *  Cache optimization example
 *  Based on cpp conference: https://www.youtube.com/watch?v=Nz9SiF0QVKY
 */

#include<iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>

#define DRAW_MS 2
#define AREA_MS 12

#define CIRCLE_DRAW_IMPL_MS DRAW_MS
#define CIRCLE_AREA_IMPL_MS AREA_MS

#define SQUARE_DRAW_IMPL_MS DRAW_MS
#define SQUARE_AREA_IMPL_MS AREA_MS

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
    virtual float area() = 0;

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
        std::this_thread::sleep_for(std::chrono::milliseconds(CIRCLE_DRAW_IMPL_MS));
    }

    float area() override
    {
        // std::cout << "Circle::area" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(CIRCLE_AREA_IMPL_MS));
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
        std::this_thread::sleep_for(std::chrono::milliseconds(SQUARE_DRAW_IMPL_MS));
    }

    float area() override
    {
        // std::cout << "Square::area" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(SQUARE_AREA_IMPL_MS));
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
        common::ScopedTimerMs timer("[cache_non_friendly] draw");
        for(const auto& shape : shapes)
        {
            shape->draw();
        }
    }

    {
        common::ScopedTimerMs timer("[cache_non_friendly] area");
        float total_area = 0;
        for(const auto& shape : shapes)
        {
            total_area += shape->area();
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
    ShapesGeometry(const uint size)
    {
        this->circles.reserve(size);
        this->squares.reserve(size);
    }
};

enum class ShapeKind : int8_t
{
    Circle = 0,
    Square
};

struct ShapeID
{
    ShapeKind kind;
    std::size_t index;
};

struct ShapeRender
{
    using shape = std::pair<ShapeID, common::Color>;
    std::vector<shape> visible;
    ShapeRender(const uint size)
    {
        this->visible.reserve(2 * size);
    }
};

void draw(const ShapeRender& render, const ShapesGeometry& geometry)
{
    common::ScopedTimerMs timer("[cache_friendly] draw");
    for(const auto& shape : render.visible)
    {
        if(shape.first.kind == ShapeKind::Circle)
        {
            // draw_circle(geometry.circles[shape.first.id.index], color);
            std::this_thread::sleep_for(std::chrono::milliseconds(CIRCLE_DRAW_IMPL_MS)); ///< implementation
        }
        else if(shape.first.kind == ShapeKind::Square)
        {
            // draw_square(geometry.squares[shape.first.id.index], color);
            std::this_thread::sleep_for(std::chrono::milliseconds(SQUARE_DRAW_IMPL_MS)); ///< implementation
        }
    }

}

float area(const ShapesGeometry& geometry)
{
    common::ScopedTimerMs timer("[cache_friendly] area");
    float total_area = 0.f;
    for(const auto& circle : geometry.circles)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(CIRCLE_AREA_IMPL_MS)); ///< implementation
    }

    for(const auto& square : geometry.circles)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SQUARE_AREA_IMPL_MS)); ///< implementation
    }
    return total_area;
}

void benchmark_fn(const uint size)
{
    using shape = std::pair<ShapeID, common::Color>;
    ShapesGeometry shapes(size);
    ShapeRender render(size);
    for(uint i = 0; i < size; ++i)
    {
        if(i % 2 == 0)
        {
            CircleGeometry circle;
            shapes.circles.push_back(std::move(circle));

            ShapeID circle_id;
            circle_id.index = i;
            circle_id.kind = ShapeKind::Circle;
            common::Color color;
            render.visible.emplace_back(circle_id, color);
        }
        else
        {
            SquareGeometry square;
            shapes.squares.push_back(std::move(square));

            ShapeID circle_id;
            circle_id.index = i;
            circle_id.kind = ShapeKind::Square;
            common::Color color;
            render.visible.emplace_back(circle_id, color);
        }
    }

    /// Measure time for each call
    draw(render, shapes);
    const float Result = area(shapes);
}
} ///< namespace cache_friendly

int main()
{
    cache_non_friendly::benchmark_fn(1000);
    cache_friendly::benchmark_fn(1000);
    return 0;
}