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
