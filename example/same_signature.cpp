#include <iostream>
#include "function_dispatcher.hpp"

struct Addition
{
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

struct Multiplication
{
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

int main()
{
    FunctionDispatcher fd;
    fd.Attach<Addition>([](int a, int b)
                        { return a + b; });
    fd.Attach<Multiplication>([](int a, int b)
                              { return a * b; });
    // Print 12
    std::cout << fd.Call<Multiplication>(3, 4) << std::endl;
    // Print 5
    std::cout << fd.Call<Addition>(2, 3) << std::endl;
}