#include <memory>
#include <iostream>

#include "function_dispatcher.hpp"

struct Ping
{
    using args_t = std::tuple<>;
    using return_t = void;
};

struct Pong
{
    using args_t = std::tuple<>;
    using return_t = void;
};

struct PingPong
{
    using args_t = std::tuple<int>;
    using return_t = void;
};

class ObjectA
{
public:
    ObjectA(std::shared_ptr<FunctionDispatcher> ed) : ed_(std::move(ed))
    {
        ed_->Attach<Ping>([this]()
                          { SayPing(); });
    }

    void SayPing()
    {
        std::cout << "Ping" << std::endl;
    }

private:
    std::shared_ptr<FunctionDispatcher> ed_;
};

class ObjectB
{
public:
    ObjectB(std::shared_ptr<FunctionDispatcher> ed) : ed_(std::move(ed))
    {
        ed_->Attach<Pong>([this]()
                          { SayPong(); });
    }

    void SayPong()
    {
        std::cout << "Pong" << std::endl;
    }

private:
    std::shared_ptr<FunctionDispatcher> ed_;
};

class ObjectC
{
public:
    ObjectC(std::shared_ptr<FunctionDispatcher> ed) : ed_(std::move(ed))
    {
        ed_->Attach<PingPong>([this](int times)
                              { SayPingPong(times); });
    }

    void SayPingPong(int times)
    {
        for (int i = 0; i < times; i++)
        {
            ed_->Call<Ping>();
            ed_->Call<Pong>();
        }
    }

private:
    std::shared_ptr<FunctionDispatcher> ed_;
};

int main()
{
    auto ed = std::make_shared<FunctionDispatcher>();
    ObjectA object_a{ed};
    ObjectB object_b{ed};
    ObjectC object_c{ed};
    ed->Call<PingPong>(5);
}