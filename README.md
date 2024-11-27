# Introduction

This library provides an event dispatcher, which allows events to be published through boost::signals2 and a boost::asio event loop
It is also possible to call functions, this will use std::function without going through the event loop

# Examples

## Simple call

```c++
#include "dispatcher.hpp"

#include <iostream>

struct Addition
{
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

int main()
{
    dispatcher::attach<Addition>([](int a, int b)
                                        {return a + b;});
    //Prints 5
    std::cout << dispatcher::call<Addition>(3, 2) << std::endl;
}
```

## Simple event

```c++
#include "dispatcher.hpp"
#include <iostream>
#include <string>

struct AnEvent {
    using args_t = std::tuple<const std::string&>;
};


int main()
{
    dispatcher::subscribe<AnEvent>([](const std::string& message)
                                      {std::cout << message << std::endl;});
    const std::string message{"Hello world"};
    dispatcher::publish<AnEvent>(message) << std::endl;
}
```

# Installation

This library is header only, but requires boost::asio and boost::signals2. You can either add this as a git submodule and link with function_dispatcher, or provide boost yourself and just include the header file.

This requires c++17, but a c++14 version is also provided

# Motivation

You can design your codebase as a set of independant code islands. This prevents the code to land in the traditional OOP pitfalls (Coupling between objects, layering, boilerplate ...).

Each code island only needs to know about its domain specific requirement, not about the rest of the program. 

# Performances 

Direct function calls are just a std::function in a trenchcoat, should be relatively easy on performances. This may confuse the compiler and remove optimizations though.
Event calls are much more expensive, since you may need to wake up a thread, and is using boost::signals2.

## Benchmark

A quick benchmark can be found in benchmark/benchmark.cpp. Here are the results:

```
Running ./function_dispatcher_benchmark
Run on (14 X 400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x7)
  L1 Instruction 64 KiB (x7)
  L2 Unified 2048 KiB (x7)
  L3 Unified 12288 KiB (x1)
Load Average: 0.35, 0.36, 0.50
-----------------------------------------------------------------------------------------
Benchmark                                               Time             CPU   Iterations
-----------------------------------------------------------------------------------------
CallAdditionDirectly                                0.503 ns        0.503 ns   1312037441
CallAdditionVirtual                                 0.985 ns        0.985 ns    712798714
CallAdditionFunctionDispatcher                      0.943 ns        0.943 ns    692453805
CallManipulateStringDirectly                         31.4 ns         31.4 ns     25463989
CallManipulateStringVirtual                          30.6 ns         30.6 ns     21954286
CallManipulateStringFunctionDispatcher               30.0 ns         30.0 ns     23685180
CallManipulateStringRefDirectly                     0.344 ns        0.344 ns   2039205455
CallManipulateStringRefVirtual                      0.882 ns        0.882 ns    803946975
CallManipulateStringRefFunctionDispatcher           0.883 ns        0.883 ns    786699368
CallManipulateStringRefFunctionDispatcherEvent        306 ns          302 ns      2273141
``` 

This is only a micro benchmark, results are indicative at best. 

# Safety 

## Compile time

You will get a compile error if their is a mistach between the function signature you defined, and the callback :

This will not compile:
```c++
#include <string>

#include "dispatcher.hpp"

struct Addition
{
    using args_t = std::tuple<float, float>;
    using return_t = float;
};

int main() {
    //Wrong return value
    dispatcher::attach<Addition>([](float, float) -> std::string {return std::string{"Hello world"};});
}
``` 

Similarly, a mistach in a call will also not compile:

```c++
#include <string>

#include "dispatcher.hpp"

struct Addition
{
    using args_t = std::tuple<float, float>;
    using return_t = float;
};

int main() {
    dispatcher::attach<Addition>([](float a, float b) -> float {return a + b;});
    // Wrong arguments
    dispatcher::call<Addition>(std::string{"Hello"}, 4.0f);
}
``` 

## Runtime 

Checking if a callback corresponds to a specific call will only be done at runtime, and will throw a std::bad_function_call exception if that is not the case

```c++
#include "dispatcher.hpp"

struct Addition
{
    using args_t = std::tuple<float, float>;
    using return_t = float;
};

int main()
{
    try
    {
        dispatcher::call<Addition>(4.0f, 3.5f);
    }
    catch (const std::bad_function_call &e)
    {
        std::cout << e.what() << std::endl;
    }
}

```





