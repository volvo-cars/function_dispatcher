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