#include <boost/fiber/future.hpp>
#include <iostream>
#include <thread>

#include "dispatcher.hpp"

bool TestWait()
{
    boost::fibers::promise<bool> hello;
    auto future = hello.get_future();
    future.wait_for(std::chrono::seconds(2));
    return true;
}

int main(int argc, char** argv)
{
    dispatcher::DefaultTimer timer;
    timer.DoEvery(std::chrono::milliseconds(500), [] {
        bool b = TestWait();
        std::cout << "Value of b : " << std::to_string(b) << std::endl;
    });
    std::this_thread::sleep_for(std::chrono::seconds{10});
}