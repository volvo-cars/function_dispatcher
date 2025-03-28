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

#include <condition_variable>
#include <dispatcher.hpp>
#include <mutex>

struct Addition {
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

int main()
{
    dispatcher::attach<Addition>([](int a, int b) { return a + b; });

    uint64_t max_tasks = 100000;

    for (int j = 0; j < 2; j++) {
        uint64_t started_tasks = 0;
        uint64_t finished_tasks = 0;

        std::mutex m;
        std::condition_variable cv;

        auto started_posting = std::chrono::system_clock::now();

        for (auto i = 0; i < max_tasks; i++) {
            dispatcher::post([&] {
                if (++started_tasks >= max_tasks) {
                    cv.notify_one();
                }
                dispatcher::async_call<Addition>(1, 3).wait();
                if (++finished_tasks >= max_tasks) {
                    cv.notify_one();
                }
            });
        }
        std::cout << "finished posting" << std::endl;
        auto finished_posting = std::chrono::system_clock::now();

        std::unique_lock<std::mutex> l{m};
        cv.wait(l);

        std::cout << "everything started" << std::endl;
        auto finished_starting_task = std::chrono::system_clock::now();
        cv.wait(l);
        std::cout << "everything finished" << std::endl;
        auto finished = std::chrono::system_clock::now();

        auto posting_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(finished_posting - started_posting).count();
        auto starting_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(finished_starting_task - finished_posting).count();
        auto finishing_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(finished - finished_starting_task).count();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started_posting).count();

        std::cout << "Time to post all tasks: " << posting_duration << " ms" << std::endl;
        std::cout << "Time to start all tasks: " << starting_duration << " ms" << std::endl;
        std::cout << "Time to finish all tasks: " << finishing_duration << " ms" << std::endl;
        std::cout << "Total time: " << total_duration << " ms" << std::endl;

        double posts_per_seconds = (max_tasks / (total_duration / 1000.0));
        // We do two posts per tasks
        std::cout << "Number of finished posts per seconds : " << posts_per_seconds * 2 << std::endl;
    }

    return 0;
}
