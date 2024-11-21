# Introduction

This C++ library provides a function dispatcher utility. You can attach callbacks to function signatures, which can then be called.
You can also publish events, which will be received by every subscriber. 

These are only direct calls, so no events loop or async calls

# Examples

## Hello world

From example/hello_world.cpp:

```c++
#include "dispatcher.hpp"

#include <iostream>
#include <string>

struct HelloWorld
{
    using args_t = std::tuple<>;
    using return_t = void;
};

int main()
{
    dispatcher::attach<HelloWorld>([]()
                                        { std::cout << "Hello world" << std::endl; });
    dispatcher::call<HelloWorld>();
}
```

## Same function signature

You can attach callbacks with the same function signature, it will not be mixed. 
The functions can also return any type.

From example/same_signature.cpp:

```c++
#include <iostream>
#include "dispatcher.hpp"

struct Addition
{
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

struct Multiplication
{
    using args_t = std::tuple<int, int>;
    using return_t = int;
};

int main()
{
    dispatcher::attach<Addition>([](int a, int b)
                                      { return a + b; });
    dispatcher::attach<Multiplication>([](int a, int b)
                                            { return a * b; });
    // Print 12
    std::cout << dispatcher::call<Multiplication>(3, 4) << std::endl;
    // Print 5
    std::cout << dispatcher::call<Addition>(2, 3) << std::endl;
}
```

## Passing references or const references

You can pass references or const references just as easy :
From example/reference.cpp

```c++
#include "dispatcher.hpp"

#include <string>
#include <iostream>

struct SayHello
{
    using args_t = std::tuple<const std::string &>;
    using return_t = void;
};

struct ModifyString
{
    using args_t = std::tuple<std::string &>;
    using return_t = void;
};

int main()
{
    dispatcher::attach<SayHello>([](
                                          const std::string &message)
                                      { std::cout << message << std::endl; });

    const std::string message{"Hello world"};
    // Prints Hello world
    dispatcher::call<SayHello>(message);

    dispatcher::attach<ModifyString>([](std::string &to_modify)
                                          { to_modify = "Hello world again"; });
    std::string message_to_modify = "";
    dispatcher::call<ModifyString>(message_to_modify);
    // Prints Hello world again
    std::cout << message_to_modify << std::endl;
}
```

# Installation

This is a header only library. Copy dispatcher.hpp and you are good to go

This requires c++ 17, although it is possible to use on c++ 11 with small modifications

# Motivation

You can design your codebase as a set of independant code islands. This prevents the code to land in the traditional OOP pitfalls (Coupling between objects, layering, boilerplate ...).

Each code island only needs to know about its domain specific requirement, not about the rest of the program. 

# Performances 

The function dispatcher is just a std::function in a trenchcoat, should be relatively easy on performances

## Benchmark

A quick benchmark can be found in benchmark/benchmark.cpp. Here are the results:

```
Running ./dispatcher_benchmark
Run on (14 X 400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x7)
  L1 Instruction 64 KiB (x7)
  L2 Unified 2048 KiB (x7)
  L3 Unified 12288 KiB (x1)
Load Average: 0.92, 0.67, 0.70
------------------------------------------------------------------------------------
Benchmark                                          Time             CPU   Iterations
------------------------------------------------------------------------------------
CallAdditionDirectly                           0.503 ns        0.503 ns   1000000000
CallAdditionVirtual                            0.947 ns        0.947 ns    761695864
CallAdditionFunctionDispatcher                  1.12 ns         1.12 ns    605790974
CallManipulateStringDirectly                    27.0 ns         27.0 ns     25914377
CallManipulateStringVirtual                     28.2 ns         28.2 ns     24682358
CallManipulateStringFunctionDispatcher          29.2 ns         29.2 ns     23930063
CallManipulateStringRefDirectly                0.359 ns        0.359 ns   2077746138
CallManipulateStringRefVirtual                 0.858 ns        0.858 ns    821195304
CallManipulateStringRefFunctionDispatcher      0.873 ns        0.873 ns    798464785
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





