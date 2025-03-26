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

#include "dispatcher.hpp"

struct Addition {
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

struct WorkNetwork {};

int main()
{
    dispatcher::getEventLoop<WorkNetwork>().SetWorkerThreadsAmount(10);
    dispatcher::attach<Addition>([](int a, int b) {
        // Let's pretend this takes work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return a + b;
    });
    std::condition_variable cv;
    dispatcher::post([&cv] {
        auto start = std::chrono::system_clock::now();
        std::vector<boost::fibers::future<int>> futures;
        for (int i = 0; i < 100; i++) {
            futures.emplace_back(dispatcher::async_call<Addition, WorkNetwork>(i, i + 1));
        }
        for (auto& future : futures) {
            future.wait();
            std::cout << "Value of future : " << future.get() << std::endl;
        }
        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Time elapsed: " << duration << " milliseconds" << std::endl;
        cv.notify_one();
    });
    std::mutex m;
    std::unique_lock<std::mutex> ul(m);
    cv.wait(ul);
}