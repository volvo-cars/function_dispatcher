#include <boost/fiber/future.hpp>
#include <iostream>
#include <thread>

#include "dispatcher.hpp"

void printTime(std::string message, int i)
{
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_time_t);

    // Extract milliseconds
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Print current time in a nice format with milliseconds
    std::cout << "timer : " << std::to_string(i) << message << std::put_time(&now_tm, "%H:%M:%S") << '.'
              << std::setfill('0') << std::setw(3) << now_ms.count() << std::endl;
}

bool TestWait(int i)
{
    printTime(" started ", i);
    boost::fibers::promise<bool> hello;

    auto future = hello.get_future();
    future.wait_for(std::chrono::seconds(2));
    printTime(" ended ", i);
    return true;
}

int main(int argc, char** argv)
{
    dispatcher::DefaultTimer timer;
    int i = 0;
    timer.DoEvery(std::chrono::milliseconds(500), [&] { bool b = TestWait(i++); });
    std::this_thread::sleep_for(std::chrono::seconds{6});
}