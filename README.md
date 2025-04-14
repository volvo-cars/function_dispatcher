# Table of Contents <!-- omit from toc -->

- [Introduction](#introduction)
  - [Key Features:](#key-features)
- [Simple examples](#simple-examples)
  - [Blocking call](#blocking-call)
  - [Event dispatching](#event-dispatching)
- [Installation](#installation)
- [Motivation](#motivation)
- [Safety](#safety)
  - [Compile time](#compile-time)
  - [Runtime](#runtime)

# Introduction

This C++ library provides asynchronous capabilities with an API designed to enable complete decoupling between software components.

## Key Features:
- **Blocking Function Calls**: Call functions using only their signature, without requiring direct references to objects.
- **Asynchronous Function Calls**: Execute functions asynchronously and retrieve results using `boost::future`, all by leveraging function signatures.
- **Thread-Safe Work Distribution**: Easily send work packages to worker threads and wait for their results in a thread-safe manner.
- **Event Dispatching**: Publish events that are received asynchronously by all subscribers.
- **Timers**: Simplify time-based operations with built-in timer support.
- **Integrated Test Framework**: Includes a test framework with capabilities and syntax similar to Google Test/Mock, making it easy to integrate the dispatcher into unit tests.

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

This library enables asynchronous development using Boost libraries in a more user-friendly and intuitive way. By decoupling your codebase into independent, self-contained "code islands," you can achieve several key benefits:

- **Reduced Coupling**: Components are isolated and do not depend on each other, making the codebase easier to maintain, extend, and refactor.
- **Improved Modularity**: Each component focuses solely on its domain-specific requirements, enhancing clarity, reusability, and separation of concerns.
- **Simplified Testing**: Decoupled components are easier to test in isolation, leading to more robust and reliable software.
- **Avoidance of Common OOP Pitfalls**: Issues such as tight coupling, excessive layering, and boilerplate code are minimized, resulting in cleaner and more maintainable code.
- **Thread-Safe Asynchronous Programming**: The library simplifies the implementation of thread-safe asynchronous programming, enabling efficient work distribution across threads.

By adopting this approach, you can design a clean, modular architecture that improves the overall quality, maintainability, and scalability of your software.

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
    dispatcher::attach<Addition>([](float, float) -> std::string { return std::string{"Hello world"}; });
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
    dispatcher::attach<Addition>([](float a, float b) -> float { return a + b; });
    // Wrong arguments
    dispatcher::call<Addition>(std::string{"Hello"}, 4.0f);
}
``` 

## Runtime 

Checking if a callback corresponds to a specific call will only be done at runtime, and will throw a NoHandler<FuncSignature> exception if that is not the case.
It is deriving from DispatcherException, which can be used to catch any NoHandler<T> exception

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
    catch (const dispatcher::NoHandler<Addition> &e)
    {
        std::cout << e.what() << std::endl;
    }
}

```





