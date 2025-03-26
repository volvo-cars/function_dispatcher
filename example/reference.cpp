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

// You can call functions and event with references, they will be perfectly forwarded

struct SayHello {
    using args_t = std::tuple<const std::string &>;
};

struct ModifyString {
    using args_t = std::tuple<std::string &>;
};

struct GetRef {
    using return_t = const std::string &;
};

int main()
{
    dispatcher::attach<SayHello>([](const std::string &message) { std::cout << message << std::endl; });

    const std::string message{"Hello world"};
    dispatcher::call<SayHello>(message);

    dispatcher::attach<ModifyString>([](std::string &to_modify) { to_modify = "Hello world again"; });
    std::string message_to_modify = "";
    dispatcher::call<ModifyString>(message_to_modify);

    std::cout << message_to_modify << std::endl;

    std::string another_message("Hello world one more time");
    // WARNING, if you are returning a const ref, you NEED to specify the return value of the lambda
    dispatcher::attach<GetRef>([&]() -> const std::string & { return another_message; });
    std::cout << dispatcher::call<GetRef>() << std::endl;
}
