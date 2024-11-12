#include "event_dispatcher_2.hpp"

#include <iostream>
#include <string>

struct HelloWorld
{
    using args_t = std::tuple<>;
    using return_t = void;
};

void SayHello()
{
    std::cout << "Hello world" << std::endl;
}

int main()
{
    EventDispatcher2 ed;
    ed.Attach<HelloWorld>(&SayHello);
    ed.Call<HelloWorld>();
}