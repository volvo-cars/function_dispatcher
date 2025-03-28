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

struct HelloWorld {};

// If you really need to, you can detach from an event

int main()
{
    dispatcher::attach<HelloWorld>([]() { std::cout << "Hello world" << std::endl; });
    dispatcher::call<HelloWorld>();
    dispatcher::detach<HelloWorld>();
    try {
        dispatcher::call<HelloWorld>();
    } catch (const dispatcher::NoHandler<HelloWorld> &e) {
        std::cout << "The function has been detached" << std::endl;
        std::cout << e.what() << std::endl;
    }
}