#include <benchmark/benchmark.h>
#include "event_dispatcher.hpp"

struct Addition
{
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

static void CallAdditionEventDispatcher(benchmark::State &state)
{
    EventDispatcher ed;
    ed.Attach<Addition>([](int a, int b)
                        { return a + b; });
    for (auto _ : state)
    {
        volatile auto i = ed.Call<Addition>(2, 3);
    }
}

class Base
{
public:
    virtual int Addition(int a, int b) = 0;
};

class Derive : public Base
{
public:
    int Addition(int a, int b) override { return a + b; }
};

static void CallAdditionVirtual(benchmark::State &state)
{
    Base *object = new Derive();
    for (auto _ : state)
    {
        volatile auto i = object->Addition(2, 3);
    }
}

BENCHMARK(CallAdditionVirtual);
BENCHMARK(CallAdditionEventDispatcher);
BENCHMARK_MAIN();