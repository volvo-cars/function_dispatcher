#include "function_dispatcher.hpp"

#include <iostream>
#include <string>

struct HelloWorld
{
    using args_t = std::tuple<>;
    using return_t = void;
};

int main()
{
    FunctionDispatcher fd;
    fd.Attach<HelloWorld>([]()
                          { std::cout << "Hello world" << std::endl; });
    fd.Call<HelloWorld>();
}