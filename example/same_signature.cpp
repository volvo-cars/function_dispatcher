#include <iostream>

#include "dispatcher.hpp"

// You can have different events with the same signature

struct Addition {
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

struct Multiplication {
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

int main()
{
    dispatcher::attach<Addition>([](int a, int b) { return a + b; });
    dispatcher::attach<Multiplication>([](int a, int b) { return a * b; });
    // Print 12
    std::cout << dispatcher::call<Multiplication>(3, 4) << std::endl;
    // Print 5
    std::cout << dispatcher::call<Addition>(2, 3) << std::endl;
}