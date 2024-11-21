#include "dispatcher.hpp"

#include <iostream>

struct HelloWorld
{
    using args_t = std::tuple<>;
    using return_t = void;
};

int main()
{
    dispatcher::attach<HelloWorld>([]()
                                   { std::cout << "Hello world" << std::endl; });
    dispatcher::call<HelloWorld>();
}