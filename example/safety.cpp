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
#include <string>

#include "dispatcher.hpp"

struct Addition {
    using args_t = std::tuple<float, float>;
    using return_t = float;
};

int main()
{
    // Missing handler will throw a NoHandler exception
    try {
        dispatcher::call<Addition>(4.0f, 3.5f);
    } catch (const dispatcher::NoHandler<Addition> &e) {
        std::cout << e.what() << std::endl;
    }

    // You can also catch all no handler exception
    try {
        dispatcher::call<Addition>(4.0f, 3.5f);
    } catch (const dispatcher::DispatcherException &e) {
        std::cout << e.what() << std::endl;
    }

    // Will not compile due when the Attach function signature does not correspond
    // dispatcher::attach<Addition>([](float a, float b) -> std::string
    //                                   { return std::string{"Hello world"}; });

    // Will not compile when the Call does not have the correct arguments
    // std::cout << dispatcher::call<Addition>(4.0f, std::string{"Hello"}) << std::endl;
}