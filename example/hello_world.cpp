#include "event_dispatcher.hpp"

#include <iostream>
#include <string>

struct HelloWorld
{
    using args_t = std::tuple<>;
    using return_t = void;
};

int main()
{
    EventDispatcher ed;
    ed.Attach<HelloWorld>([]()
                          { std::cout << "Hello world" << std::endl; });
    ed.Call<HelloWorld>();
}