#include <iostream>
#include "event_dispatcher.hpp"
#include <boost/any.hpp>

struct Addition
{
    using args = std::tuple<int, int>;
    using return_type = int;
};

struct Multiplication
{
    using args = std::tuple<int, int>;
    using return_type = int;
};

int main()
{
    EventDispatcher ed;
    ed.Attach<Addition>([](int a, int b)
                        { return a + b; });
    ed.Attach<Multiplication>([](int a, int b)
                              { return a * b; });
    // Print 12
    std::cout << ed.Call<Multiplication>(3, 4) << std::endl;
    // Print 5
    std::cout << ed.Call<Addition>(2, 3) << std::endl;
}