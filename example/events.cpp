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

#include <iostream>

#include "dispatcher.hpp"

// You can have multiple networks, each one with their own event loop

struct AnEvent {
    using args_t = std::tuple<int, int>;
};

struct OtherNetwork {};

int main()
{
    dispatcher::subscribe<AnEvent>([](int a, int b) {
        std::cout << "Received an event on default network subscriber 1; a: " << a << " b: " << b << std::endl;
    });
    dispatcher::subscribe<AnEvent>([](int a, int b) {
        std::cout << "Received an event on default network subscriber 2; a: " << a << " b: " << b << std::endl;
    });
    dispatcher::subscribe<AnEvent, OtherNetwork>([](int a, int b) {
        std::cout << "Received an event on other network subscriber 3; a: " << a << " b: " << b << std::endl;
    });
    dispatcher::publish<AnEvent>(1, 2);
    dispatcher::publish<AnEvent, OtherNetwork>(3, 4);
}