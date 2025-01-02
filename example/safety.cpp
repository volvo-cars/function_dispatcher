#include <iostream>
#include <string>

#include "dispatcher.hpp"

struct Addition
{
    using args_t = std::tuple<float, float>;
    using return_t = float;
};

DEFINE_FUNCTION_DISPATCHER(Addition)

int main()
{
    // Missing handler will throw a std::bad_function_call exception
    try
    {
        dispatcher::call<Addition>(4.0f, 3.5f);
    }
    catch (const std::bad_function_call &e)
    {
        std::cout << e.what() << std::endl;
    }
    // Will not compile due when the Attach function signature does not correspond
    // dispatcher::attach<Addition>([](float a, float b) -> std::string
    //                                   { return std::string{"Hello world"}; });

    // Will not compile when the Call does not have the correct arguments
    // std::cout << dispatcher::call<Addition>(4.0f, std::string{"Hello"}) << std::endl;
}