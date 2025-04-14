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

#include "dispatcher.hpp"

struct AnEvent {
    using parameters_t = std::tuple<std::string>;
};

int main()
{
    dispatcher::post([] {
        auto future = dispatcher::expect<AnEvent>();
        auto future_2 = dispatcher::expect<AnEvent>([](auto s) { std::cout << s << std::endl; });
        dispatcher::publish<AnEvent>("Hello world");
        // Will block until event is received
        future.wait();
        // The subscribtion are fullfilled when the event is done
        dispatcher::publish<AnEvent>("Hello world");
    });
    std::this_thread::sleep_for(std::chrono::seconds{1});
}
