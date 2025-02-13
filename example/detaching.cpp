#include <iostream>

#include "dispatcher.hpp"

struct HelloWorld {};

// If you really need to, you can detach from an event

int main()
{
    dispatcher::attach<HelloWorld>([]() { std::cout << "Hello world" << std::endl; });
    dispatcher::call<HelloWorld>();
    dispatcher::detach<HelloWorld>();
    try {
        dispatcher::call<HelloWorld>();
    } catch (const dispatcher::NoHandler<HelloWorld> &) {
        std::cout << "The function has been detached" << std::endl;
    }
}