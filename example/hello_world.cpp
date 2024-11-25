#include "dispatcher.hpp"

#include <iostream>

struct HelloWorld
{
};

int main()
{
    dispatcher::attach<HelloWorld>([]()
                                   { std::cout << "Hello world" << std::endl; });
    dispatcher::call<HelloWorld>();
}