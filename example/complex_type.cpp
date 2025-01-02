#include "dispatcher.hpp"

#include <string>
#include <iostream>

struct Message
{
    Message(std::string _message) : message(std::move(_message)) { std::cout << "Normal constructor called" << std::endl; }
    Message(const Message &other) : message(other.message) { std::cout << "Copy constructor called" << std::endl; }
    Message &operator=(const Message &other)
    {
        message = other.message;
        std::cout << "Copy constructor called" << std::endl;
        return *this;
    }
    Message(Message &&other) : message(std::move(other.message)) { std::cout << "Move constructor called" << std::endl; }
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

struct MoveComplexType
{
    using args_t = std::tuple<Message, std::string>;
    using return_t = Message;
};

DEFINE_FUNCTION_DISPATCHER(MoveComplexType)

// Perfect forwarding all the way

int main()
{
    dispatcher::attach<MoveComplexType>([](
                                            Message message, std::string message_to_add)
                                        { message.message.append(message_to_add);
                               std::cout << "Returning message" << std::endl;
                               return message; });
    std::cout << dispatcher::call<MoveComplexType>(Message{"Hello "}, std::string{"world"}).message << std::endl;
}
