#include <benchmark/benchmark.h>
#include "event_dispatcher.hpp"
#include "function_dispatcher.hpp"

namespace bm = benchmark;

class Base
{
public:
    virtual int Addition(int a, int b) = 0;
    virtual std::string ManipulateString(std::string) = 0;
    virtual void ManipulateStringRef(const std::string &) = 0;
};

class Derive : public Base
{
public:
    int Addition(int a, int b) override { return a + b; }
    std::string ManipulateString(std::string a)
    {
        a.append(" world");
        return a;
    };
    void ManipulateStringRef(const std::string &a)
    {
        bm::DoNotOptimize(a.size());
    }
};

class Derive_2 : public Base
{
public:
    int Addition(int a, int b) override { return a + b; }
    std::string ManipulateString(std::string a)
    {
        a.append(" planet");
        return a;
    };
    void ManipulateStringRef(const std::string &a)
    {
        bm::DoNotOptimize(a.size());
    }
};

static void CallAdditionDirectly(benchmark::State &state)
{
    std::vector<Derive *> objects;
    objects.push_back(new Derive());

    for (auto _ : state)
    {
        for (Derive *base : objects)
        {
            bm::DoNotOptimize(base->Addition(1, 2));
        }
    }
}

static void CallAdditionVirtual(benchmark::State &state)
{
    // Disable inlining
    std::vector<Base *> objects;
    objects.push_back(new Derive());

    for (auto _ : state)
    {
        for (Base *base : objects)
        {
            bm::DoNotOptimize(base->Addition(1, 2));
        }
    }
}

struct Addition
{
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

static void CallAdditionEventDispatcher(benchmark::State &state)
{
    EventDispatcher fd;
    fd.Attach<Addition>([](int a, int b)
                        { return a + b; });
    for (auto _ : state)
    {
        bm::DoNotOptimize(fd.Call<Addition>(2, 3));
    }
}

int AdditionCall(int a, int b)
{
    return a + b;
}

static void CallAdditionFunctionDispatcher(benchmark::State &state)
{
    FunctionDispatcher fd;
    fd.Attach<Addition>(&AdditionCall);
    for (auto _ : state)
    {
        bm::DoNotOptimize(fd.Call<Addition>(2, 3));
    }
}

static void CallManipulateStringDirectly(benchmark::State &state)
{
    std::vector<Derive *> objects;
    objects.push_back(new Derive());

    for (auto _ : state)
    {
        bm::DoNotOptimize(objects[0]->ManipulateString(std::string("Hello this is a long string, really long string. Let's make sure it is too long to be optinized away")));
    }
}

static void CallManipulateStringVirtual(benchmark::State &state)
{
    // Disable inlining
    std::vector<Base *> objects;
    objects.push_back(new Derive());

    for (auto _ : state)
    {
        bm::DoNotOptimize(objects[0]->ManipulateString(std::string("Hello this is a long string, really long string. Let's make sure it is too long to be optinized away")));
    }
}

struct ManipulateString
{
    using args_t = std::tuple<std::string>;
    using return_t = std::string;
};

static void CallManipulateStringEventDispatcher(benchmark::State &state)
{
    EventDispatcher fd;
    fd.Attach<ManipulateString>([](std::string a)
                                {a.append(" planet");
        return a; });
    for (auto _ : state)
    {
        bm::DoNotOptimize(fd.Call<ManipulateString>(std::string{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"}));
    }
}

std::string ManipulateStringCall(std::string message)
{
    message.append(" planet");
    return message;
}

static void CallManipulateStringFunctionDispatcher(benchmark::State &state)
{
    FunctionDispatcher fd;
    fd.Attach<ManipulateString>(&ManipulateStringCall);
    for (auto _ : state)
    {
        bm::DoNotOptimize(fd.Call<ManipulateString>(std::string{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"}));
    }
}

static void CallManipulateStringRefVirtual(benchmark::State &state)
{
    // Disable inlining
    std::vector<Base *> objects;
    objects.push_back(new Derive());
    const std::string a{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};

    for (auto _ : state)
    {

        objects[0]->ManipulateStringRef(a);
    }
}

static void CallManipulateStringRefDirectly(benchmark::State &state)
{
    std::vector<Derive *> objects;
    objects.push_back(new Derive());
    const std::string a{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};

    for (auto _ : state)
    {
        objects[0]->ManipulateStringRef(a);
    }
}

struct ManipulateStringRef
{
    using args_t = std::tuple<std::reference_wrapper<const std::string>>;
    using return_t = void;
};

void ManipulateStringRefCall(std::reference_wrapper<const std::string> a)
{
    bm::DoNotOptimize(a.get().size());
}

static void CallManipulateStringRefEventDispatcher(benchmark::State &state)
{
    EventDispatcher fd;
    fd.Attach<ManipulateStringRef>(&ManipulateStringRefCall);
    const std::string a{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};
    for (auto _ : state)
    {
        fd.Call<ManipulateStringRef>(std::ref(a));
    }
}

static void CallManipulateStringRefFunctionDispatcher(benchmark::State &state)
{
    FunctionDispatcher fd;
    fd.Attach<ManipulateStringRef>(&ManipulateStringRefCall);
    const std::string a{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};
    for (auto _ : state)
    {
        fd.Call<ManipulateStringRef>(std::ref(a));
    }
}

BENCHMARK(CallAdditionDirectly);
BENCHMARK(CallAdditionVirtual);
BENCHMARK(CallAdditionFunctionDispatcher);
BENCHMARK(CallAdditionEventDispatcher);
BENCHMARK(CallManipulateStringDirectly);
BENCHMARK(CallManipulateStringVirtual);
BENCHMARK(CallManipulateStringFunctionDispatcher);
BENCHMARK(CallManipulateStringEventDispatcher);
BENCHMARK(CallManipulateStringRefDirectly);
BENCHMARK(CallManipulateStringRefVirtual);
BENCHMARK(CallManipulateStringRefFunctionDispatcher);
BENCHMARK(CallManipulateStringRefEventDispatcher);
BENCHMARK_MAIN();