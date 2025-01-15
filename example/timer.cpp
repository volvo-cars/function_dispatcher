#include <iostream>
#include <thread>

#include "dispatcher.hpp"

int main()
{
    dispatcher::DefaultTimer timer;
    timer.DoIn([] { std::cout << "Hello" << std::endl; }, boost::posix_time::seconds{1});
    dispatcher::DefaultTimer timer_2;
    timer_2.DoIn([] { std::cout << "Wirld" << std::endl; }, boost::posix_time::seconds{2});
    dispatcher::DefaultTimer timer_3;
    timer_3.DoIn([] { std::cout << "World" << std::endl; }, boost::posix_time::seconds{3});

    timer_2.Cancel();
    std::this_thread::sleep_for(std::chrono::seconds{4});
}