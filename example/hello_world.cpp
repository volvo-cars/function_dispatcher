#include "event_dispatcher.hpp"

#include <iostream>
#include <string>

struct HelloWorld
{
    using args = std::tuple<>;
    using return_type = void;
};

int main()
{
    EventDispatcher ed;
    ed.Attach<HelloWorld>([]()
                          { std::cout << "Hello world" << std::endl; });
    ed.Call<HelloWorld>();
}