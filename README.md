# Table of Contents <!-- omit from toc -->

- [Introduction](#introduction)
- [Simple examples](#simple-examples)
  - [Blocking call](#blocking-call)
  - [Event dispatching](#event-dispatching)
- [Installation](#installation)
- [Motivation](#motivation)
- [Performances](#performances)
  - [Blocking calls benchmark](#blocking-calls-benchmark)
  - [Async calls benchmark](#async-calls-benchmark)
- [Safety](#safety)
  - [Compile time](#compile-time)
  - [Runtime](#runtime)

# Introduction

This C++ library aims at providing async capabilities, with an API that allows complete decoupling between software components.

It provides the following features:

- Blocking function calling by only using function signature
- Async function calling returning `boost::future`, by only using function signature
- Easily sending work packages to worker threads and waiting for the result
- Event dispatching
- Timer
- Test framework with similar capabilities/syntax as Google Test/Mock, to integrate the dispatcher in unit tests

# Simple examples

## Blocking call

Using the attach/call API, it is possible to call functions without needing a reference to an object or a callback. 

```c++
#include "dispatcher.hpp"

#include <iostream>

// This is the function "signature"
struct Addition
{
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

int main()
{
    // Code in component A
    dispatcher::attach<Addition>([](int a, int b)
                                        {return a + b;});
    //Blocking function, will print 5. Can be callled from component B without holding any reference to component A
    std::cout << dispatcher::call<Addition>(3, 2) << std::endl;
}
```

## Event dispatching

It is possible to publish events that will be received by every subscriber. The dispatching is done asynchronously 

```c++
#include "dispatcher.hpp"
#include <iostream>
#include <string>

struct AnEvent {
    using parameters_t = std::tuple<const std::string&>;
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

This C++ 14 library requires boost::asio and boost::signals2. You can either add this as a git submodule and link with function-dispatcher, or provide boost yourself and just include the header file.

# Motivation

You can design your codebase as a set of independant code islands. This prevents the code to land in the traditional OOP pitfalls (Coupling between objects, layering, boilerplate ...).

Each code island only needs to know about its domain specific requirement, not about the rest of the program. 

It is also really easy to accomplish async programming in a thread safe manner, or to move work packages from one thread to another.

# Performances 

Blocking function calls (dispatcher::call) are just a std::function in a trenchcoat, should be relatively easy on performances. This may confuse the compiler and remove optimizations though.
Event calls are much more expensive, since you may need to wake up a thread, and is using boost::signals2.
Async calls have similar performances as event calls.

## Blocking calls benchmark

A quick benchmark can be found in benchmark/benchmark.cpp. Here are the results:

```
Running ./function-dispatcher_benchmark
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

## Async calls benchmark

Another benchmark can be found in benchmark/custom_benchmark 

```
./custom_benchmark            
//First run of event posting, need to allocate all the stack                                         
finished posting
everything started
everything finished
Time to post all tasks: 8 ms
Time to start all tasks: 216 ms
Time to finish all tasks: 94 ms
Total time: 320 ms
Number of finished posts per seconds : 625000
 
// Second time, we have not freed the fiber stack, so it is faster
finished posting
everything started
everything finished
Time to post all tasks: 15 ms
Time to start all tasks: 68 ms
Time to finish all tasks: 104 ms
Total time: 188 ms
Number of finished posts per seconds : 1.06383e+06
```

This roughly means we can reach 1 million finished task every second, for a single threaded event loop. The event loop is slower at the beginning as we are recycling memory stacks of completed posts

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





