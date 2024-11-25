#include "dispatcher.hpp"

#include <string>
#include <iostream>

struct SayHello
{
    using args_t = std::tuple<const std::string &>;
};

struct ModifyString
{
    using args_t = std::tuple<std::string &>;
};

int main()
{
    dispatcher::attach<SayHello>([](
                                     const std::string &message)
                                 { std::cout << message << std::endl; });

    const std::string message{"Hello world"};
    dispatcher::call<SayHello>(message);

    dispatcher::attach<ModifyString>([](std::string &to_modify)
                                     { to_modify = "Hello world again"; });
    std::string message_to_modify = "";
    dispatcher::call<ModifyString>(message_to_modify);
    // Prints hello world again
    std::cout << message_to_modify << std::endl;
}
