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
        volatile auto j = ed.Call<Addition>(2, 3);
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

class Derive_2 : public Base
{
public:
    int Addition(int a, int b) override { return a + b + 1; }
};

static void CallAdditionVirtual(benchmark::State &state)
{
    Base *object = new Derive();
    Base *object_2 = new Derive_2();
    for (auto _ : state)
    {
        volatile auto i = object->Addition(2, 3);
        volatile auto j = object_2->Addition(2, 3);
    }
}

static void CallAdditionDirectly(benchmark::State &state)
{
    Derive object;
    Derive_2 object_2;
    for (auto _ : state)
    {
        volatile auto i = object.Addition(2, 3);
        volatile auto j = object_2.Addition(2, 3);
    }
}

BENCHMARK(CallAdditionVirtual);
BENCHMARK(CallAdditionEventDispatcher);
BENCHMARK(CallAdditionDirectly);
BENCHMARK_MAIN();