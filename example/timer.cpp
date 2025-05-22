// Copyright 2025 Volvo Car Corporation
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <boost/fiber/future.hpp>
#include <iostream>
#include <thread>

#include "dispatcher.hpp"

void printTime(std::string message, int i)
{
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    struct tm now_tm;
    localtime_r(&now_time_t, &now_tm);

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
    std::this_thread::sleep_for(std::chrono::seconds{6000});
}