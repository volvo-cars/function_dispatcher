#include <iostream>
#include <vector>

#include "function_dispatcher.hpp"

struct Addition
{
    using args_t = std::tuple<float, float>;
    using return_t = float;
};

int main()
{
    FunctionDispatcher fd;
    // Missing handler will throw a std::bad_function_call exception
    try
    {
        fd.Call<Addition>(4.0f, 3.5f);
    }
    catch (const std::bad_function_call &e)
    {
        std::cout << e.what() << std::endl;
    }
    // Will not compile due when the Attach function signature does not correspond
    // fd.Attach<Addition>([](float a, float b) -> std::string
    //                     { return std::string{"jzhfkjhfjkh"}; });

    // Will not compile when the Call does not have the correct arguments
    // std::cout << fd.Call<Addition>(4.0f, std::string{"Hello"}) << std::endl;
}