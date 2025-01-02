#include "dispatcher.hpp"

#include <iostream>

// You can have multiple networks, each one with their own event loop

struct AnEvent
{
    using args_t = std::tuple<int, int>;
};

DEFINE_EVENT_DISPATCHER(AnEvent)

struct OtherNetwork
{
};

DEFINE_EVENT_DISPATCHER_NETWORK(AnEvent, OtherNetwork)

int main()
{
    dispatcher::subscribe<AnEvent>([](int a, int b)
                                   { std::cout << "Received an event on default network subscriber 1; a: " << a << " b: " << b << std::endl; });
    dispatcher::subscribe<AnEvent>([](int a, int b)
                                   { std::cout << "Received an event on default network subscriber 2; a: " << a << " b: " << b << std::endl; });
    dispatcher::subscribe<AnEvent, OtherNetwork>([](int a, int b)
                                                 { std::cout << "Received an event on other network subscriber 3; a: " << a << " b: " << b << std::endl; });
    dispatcher::publish<AnEvent>(1, 2);
    dispatcher::publish<AnEvent, OtherNetwork>(3, 4);
}