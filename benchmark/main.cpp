#include <benchmark/benchmark.h>
#include "event_dispatcher.hpp"

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
    objects.push_back(new Derive_2());

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
    EventDispatcher ed;
    ed.Attach<Addition>([](int a, int b)
                        { return a + b; });
    for (auto _ : state)
    {
        bm::DoNotOptimize(ed.Call<Addition>(2, 3));
        bm::DoNotOptimize(ed.Call<Addition>(2, 3));
    }
}

static void CallManipulateStringDirectly(benchmark::State &state)
{
    std::vector<Derive *> objects;
    objects.push_back(new Derive());
    objects.push_back(new Derive());

    for (auto _ : state)
    {
        for (Derive *base : objects)
        {
            bm::DoNotOptimize(base->ManipulateString(std::string("Hello this is a long string, really long string. Let's make sure it is too long to be optinized away")));
        }
    }
}

static void CallManipulateStringVirtual(benchmark::State &state)
{
    // Disable inlining
    std::vector<Base *> objects;
    objects.push_back(new Derive());
    objects.push_back(new Derive_2());

    for (auto _ : state)
    {
        for (Base *base : objects)
        {
            bm::DoNotOptimize(base->ManipulateString(std::string("Hello this is a long string, really long string. Let's make sure it is too long to be optinized away")));
        }
    }
}

struct ManipulateString
{
    using args_t = std::tuple<std::string>;
    using return_t = std::string;
};

static void CallManipulateStringEventDispatcher(benchmark::State &state)
{
    EventDispatcher ed;
    ed.Attach<ManipulateString>([](std::string a)
                                {a.append(" planet");
        return a; });
    for (auto _ : state)
    {
        bm::DoNotOptimize(ed.Call<ManipulateString>(std::string{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"}));
    }
}

static void CallManipulateStringRefVirtual(benchmark::State &state)
{
    // Disable inlining
    std::vector<Base *> objects;
    objects.push_back(new Derive());
    objects.push_back(new Derive_2());
    const std::string a{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};

    for (auto _ : state)
    {
        for (Base *base : objects)
        {

            base->ManipulateStringRef(a);
        }
    }
}

static void CallManipulateStringRefDirectly(benchmark::State &state)
{
    std::vector<Derive *> objects;
    objects.push_back(new Derive());
    objects.push_back(new Derive());
    const std::string a{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};

    for (auto _ : state)
    {
        for (Derive *base : objects)
        {
            base->ManipulateStringRef(a);
        }
    }
}

struct ManipulateStringRef
{
    using args_t = std::tuple<std::reference_wrapper<const std::string>>;
    using return_t = void;
};

static void CallManipulateStringRefEventDispatcher(benchmark::State &state)
{
    EventDispatcher ed;
    ed.Attach<ManipulateStringRef>([](std::reference_wrapper<const std::string> a)
                                   { volatile auto i = a.get().size(); });
    const std::string a{"Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};
    for (auto _ : state)
    {
        ed.Call<ManipulateStringRef>(std::ref(a));
        ed.Call<ManipulateStringRef>(std::ref(a));
    }
}

BENCHMARK(CallAdditionDirectly);
BENCHMARK(CallAdditionVirtual);
BENCHMARK(CallAdditionEventDispatcher);
BENCHMARK(CallManipulateStringDirectly);
BENCHMARK(CallManipulateStringVirtual);
BENCHMARK(CallManipulateStringEventDispatcher);
BENCHMARK(CallManipulateStringRefDirectly);
BENCHMARK(CallManipulateStringRefVirtual);
BENCHMARK(CallManipulateStringRefEventDispatcher);
BENCHMARK_MAIN();