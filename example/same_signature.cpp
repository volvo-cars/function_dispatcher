#include <iostream>
#include "event_dispatcher.hpp"
#include <boost/any.hpp>

struct Addition
{
    using args = std::tuple<>;
    using return_type = int;
};

int main()
{
    // boost::any a = boost::any{std::tuple<>{}};
    // std::cout << sizeof(std::tuple<>{}) << std::endl;
    // std::cout << sizeof(a) << std::endl;
    // auto t = boost::any_cast<std::tuple<>>(a);
    EventDispatcher ed;
    ed.Attach<Addition>([]()
                        { return 5; });
    // ed.Attach<Multiplication>([](int a, int b)
    //                           { return a * b; });
    // Print 12
    // std::cout << ed.Call<Multiplication>(3, 4) << std::endl;
    // Print 5
    std::cout << ed.Call<Addition>() << std::endl;
}