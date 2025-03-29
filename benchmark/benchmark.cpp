// Copyright 2025 Volvo Car Corporation
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

#include <benchmark/benchmark.h>

#include "dispatcher.hpp"

namespace bm = benchmark;

class Base {
  public:
    virtual int Addition(int a, int b) = 0;
    virtual std::string ManipulateString(std::string) = 0;
    virtual void ManipulateStringRef(const std::string &) = 0;
};

class Derive : public Base {
  public:
    int Addition(int a, int b) override
    {
        return a + b;
    }
    std::string ManipulateString(std::string a) override
    {
        a.append(" world");
        return a;
    };
    void ManipulateStringRef(const std::string &a) override
    {
        bm::DoNotOptimize(a.size());
    }
};

class Derive_2 : public Base {
  public:
    int Addition(int a, int b) override
    {
        return a + b;
    }
    std::string ManipulateString(std::string a) override
    {
        a.append(" planet");
        return a;
    };
    void ManipulateStringRef(const std::string &a) override
    {
        bm::DoNotOptimize(a.size());
    }
};

static void CallAdditionDirectly(benchmark::State &state)
{
    std::vector<Derive *> objects;
    objects.push_back(new Derive());

    for (auto _ : state) {
        for (Derive *base : objects) {
            bm::DoNotOptimize(base->Addition(1, 2));
        }
    }
}

static void CallAdditionVirtual(benchmark::State &state)
{
    // Disable inlining
    std::vector<Base *> objects;
    objects.push_back(new Derive());

    for (auto _ : state) {
        for (Base *base : objects) {
            bm::DoNotOptimize(base->Addition(1, 2));
        }
    }
}

struct Addition {
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

static void CallAdditionFunctionDispatcher(benchmark::State &state)
{
    dispatcher::attach<Addition>([](int a, int b) { return a + b; });
    for (auto _ : state) {
        bm::DoNotOptimize(dispatcher::call<Addition>(2, 3));
    }
}

static void CallManipulateStringDirectly(benchmark::State &state)
{
    std::vector<Derive *> objects;
    objects.push_back(new Derive());

    for (auto _ : state) {
        bm::DoNotOptimize(objects[0]->ManipulateString(std::string(
            "Hello this is a long string, really long string. Let's make sure it is too long to be optinized away")));
    }
}

static void CallManipulateStringVirtual(benchmark::State &state)
{
    // Disable inlining
    std::vector<Base *> objects;
    objects.push_back(new Derive());

    for (auto _ : state) {
        bm::DoNotOptimize(objects[0]->ManipulateString(std::string(
            "Hello this is a long string, really long string. Let's make sure it is too long to be optinized away")));
    }
}

struct ManipulateString {
    using args_t = std::tuple<std::string>;
    using return_t = std::string;
};

static void CallManipulateStringFunctionDispatcher(benchmark::State &state)
{
    dispatcher::attach<ManipulateString>([](std::string a) {
        a.append(" planet");
        return a;
    });
    for (auto _ : state) {
        bm::DoNotOptimize(dispatcher::call<ManipulateString>(std::string{
            "Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"}));
    }
}

static void CallManipulateStringRefVirtual(benchmark::State &state)
{
    // Disable inlining
    std::vector<Base *> objects;
    objects.push_back(new Derive());
    const std::string a{
        "Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};

    for (auto _ : state) {
        objects[0]->ManipulateStringRef(a);
    }
}

static void CallManipulateStringRefDirectly(benchmark::State &state)
{
    std::vector<Derive *> objects;
    objects.push_back(new Derive());
    const std::string a{
        "Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};

    for (auto _ : state) {
        objects[0]->ManipulateStringRef(a);
    }
}

struct ManipulateStringRef {
    using args_t = std::tuple<const std::string &>;
};

static void CallManipulateStringRefFunctionDispatcher(benchmark::State &state)
{
    dispatcher::attach<ManipulateStringRef>([](const std::string &message) { bm::DoNotOptimize(message.size()); });
    const std::string a{
        "Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};
    for (auto _ : state) {
        dispatcher::call<ManipulateStringRef>(a);
    }
}

static void CallManipulateStringRefFunctionDispatcherEvent(benchmark::State &state)
{
    dispatcher::subscribe<ManipulateStringRef>([](const std::string &message) { bm::DoNotOptimize(message.size()); });
    const std::string a{
        "Hello this is a long string, really long string. Let's make sure it is too long to be optinized away"};
    for (auto _ : state) {
        dispatcher::publish<ManipulateStringRef>(a);
    }
}

BENCHMARK(CallAdditionDirectly);
BENCHMARK(CallAdditionVirtual);
BENCHMARK(CallAdditionFunctionDispatcher);
BENCHMARK(CallManipulateStringDirectly);
BENCHMARK(CallManipulateStringVirtual);
BENCHMARK(CallManipulateStringFunctionDispatcher);
BENCHMARK(CallManipulateStringRefDirectly);
BENCHMARK(CallManipulateStringRefVirtual);
BENCHMARK(CallManipulateStringRefFunctionDispatcher);
BENCHMARK(CallManipulateStringRefFunctionDispatcherEvent);
BENCHMARK_MAIN();