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

struct Message {
    Message(std::string _message) : message(std::move(_message))
    {
        std::cout << "Normal constructor called" << std::endl;
    }
    Message(const Message &other) : message(other.message)
    {
        std::cout << "Copy constructor called" << std::endl;
    }
    Message &operator=(const Message &other)
    {
        message = other.message;
        std::cout << "Copy constructor called" << std::endl;
        return *this;
    }
    Message(Message &&other) : message(std::move(other.message))
    {
        std::cout << "Move constructor called" << std::endl;
    }
    Message &operator=(Message &&other)
    {
        message = std::move(other.message);
        std::cout << "Move constructor called" << std::endl;
        return *this;
    }
    ~Message()
    {
        std::cout << "Destructor called" << std::endl;
    }
    std::string message = "";
};

struct MoveComplexType {
    using args_t = std::tuple<Message, std::string>;
    using return_t = Message;
};

// Perfect forwarding all the way

int main()
{
    dispatcher::attach<MoveComplexType>([](Message message, std::string message_to_add) {
        message.message.append(message_to_add);
        std::cout << "Returning message" << std::endl;
        return message;
    });
    std::cout << dispatcher::call<MoveComplexType>(Message{"Hello "}, std::string{"world"}).message << std::endl;
}
