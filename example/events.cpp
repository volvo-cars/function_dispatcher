#include "dispatcher.hpp"

#include <iostream>

struct HelloWorld
{
    using args_t = std::tuple<>;
};

int main()
{
    dispatcher::subscribe<HelloWorld>([]()
                                      { std::cout << "Hello world subscriber one" << std::endl; });
    dispatcher::subscribe<HelloWorld>([]()
                                      { std::cout << "Hello world subscriber two" << std::endl; });
    dispatcher::publish<HelloWorld>();
}