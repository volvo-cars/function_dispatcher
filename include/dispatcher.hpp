#pragma once

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

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/fiber/all.hpp>
#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <functional>
#include <queue>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace dispatcher {
namespace internal {

template <typename ReturnType, typename Tuple>
struct FunctionFromTuple;

template <typename ReturnType, typename... Args>
struct FunctionFromTuple<ReturnType, std::tuple<Args...>> {
    using type = std::function<ReturnType(Args...)>;
};

template <typename ReturnType, typename Tuple>
struct ReturnTypeFromCallable;

template <typename Callable, typename... Args>
struct ReturnTypeFromCallable<Callable, std::tuple<Args...>> {
    using type = typename std::result_of<Callable(Args...)>::type;
};

template <typename Tuple>
struct SignalFromTuple;

template <typename... Args>
struct SignalFromTuple<std::tuple<Args...>> {
    using type = boost::signals2::signal<void(Args...)>;
};

template <typename...>
using void_t = void;

template <typename FuncSignature, typename = void>
struct has_return_t : std::false_type {};

template <typename FuncSignature>
struct has_return_t<FuncSignature, void_t<typename FuncSignature::return_t>> : std::true_type {};

template <typename FuncSignature, bool HasReturnT>
struct return_t_or_default {
    using type = void;
};

template <typename FuncSignature>
struct return_t_or_default<FuncSignature, true> {
    using type = typename FuncSignature::return_t;
};

template <typename FuncSignature, typename = void>
struct has_args_t : std::false_type {};

template <typename FuncSignature>
struct has_args_t<FuncSignature, void_t<typename FuncSignature::args_t>> : std::true_type {};

template <typename FuncSignature, bool HasReturnT>
struct args_t_or_default {
    using type = std::tuple<>;
};

template <typename FuncSignature>
struct args_t_or_default<FuncSignature, true> {
    using type = typename FuncSignature::args_t;
};

// Default network
struct Default {};

}  // namespace internal

template <typename FuncSignature>
class NoHandler : public std::exception {
  public:
    const char *what() const noexcept override
    {
        static std::string message =
            "No handler was attached for FuncSignature: " + std::string(typeid(FuncSignature).name());
        return message.c_str();
    }
};

namespace internal {

template <typename FuncSignature, typename func_type>
func_type &GetFunction()
{
    static func_type function;
    return function;
}

template <typename FuncSignature>
struct FunctionDispatcher {
    template <typename Callable>
    static void attach(Callable &&callable)
    {
        GetFunction<FuncSignature, func_type>() = std::forward<Callable>(callable);
    }

    static void detach()
    {
        GetFunction<FuncSignature, func_type>() = nullptr;
    }

    template <typename... Args>
    static auto call(Args &&...args)
    {
        if (GetFunction<FuncSignature, func_type>() == nullptr) {
            throw NoHandler<FuncSignature>{};
        }
        return GetFunction<FuncSignature, func_type>()(std::forward<Args>(args)...);
    }

    using return_t = typename return_t_or_default<FuncSignature, has_return_t<FuncSignature>::value>::type;
    using args_t = typename args_t_or_default<FuncSignature, has_args_t<FuncSignature>::value>::type;

    using func_type = typename FunctionFromTuple<return_t, args_t>::type;
};

class MemoryPool {
  public:
    MemoryPool(std::size_t size) : size_(size)
    {
    }
    ~MemoryPool()
    {
        while (!allocated_blocks_.empty()) {
            auto memory_block = allocated_blocks_.front();
            allocated_blocks_.pop();
            std::free(memory_block);
        }
    }
    MemoryPool(const MemoryPool &) = delete;
    MemoryPool(MemoryPool &&) = delete;
    MemoryPool &operator=(const MemoryPool &) = delete;
    MemoryPool &operator=(MemoryPool &&) = delete;

    void *allocate()
    {
        if (allocated_blocks_.empty()) {
            void *memory = std::malloc(size_);
            if (!memory) {
                throw std::bad_alloc();
            }
            return memory;
        }
        auto memory = allocated_blocks_.front();
        allocated_blocks_.pop();
        return memory;
    }
    void free(void *memory_block)
    {
        allocated_blocks_.push(memory_block);
    }

    std::size_t get_size()
    {
        return size_;
    }

  private:
    std::size_t size_;
    std::queue<void *> allocated_blocks_;
};

thread_local MemoryPool memory_pool{30000};

class CustomStackAllocator {
  public:
    CustomStackAllocator()
    {
    }

    boost::context::stack_context allocate()
    {
        boost::context::stack_context sctx;
        sctx.size = memory_pool.get_size();
        sctx.sp = static_cast<char *>(memory_pool.allocate()) + sctx.size;
        return sctx;
    }

    void deallocate(boost::context::stack_context &sctx)
    {
        BOOST_ASSERT(sctx.sp);

        void *vp = static_cast<char *>(sctx.sp) - sctx.size;
        memory_pool.free(vp);
    }
};

template <typename Network>
class EventLoop {
  public:
    EventLoop()
        : work_guard_{boost::asio::make_work_guard(io_context_)}, work_thread_{[this] {
              while (!stopped_) {
                  io_context_.poll_one();
                  boost::this_fiber::yield();
              }
          }}
    {
    }
    ~EventLoop()
    {
        Stop();
    }

    EventLoop(const EventLoop &) = delete;
    EventLoop(EventLoop &&) = delete;
    EventLoop &operator=(const EventLoop &) = delete;
    EventLoop &operator=(EventLoop &&) = delete;

    void SetWorkerThreadsAmount(int threads)
    {
        while (additional_work_threads_.size() < threads - 1) {
            additional_work_threads_.emplace_back([this] {
                while (!stopped_) {
                    io_context_.poll_one();
                    boost::this_fiber::yield();
                }
            });
        }
    }

    template <typename T>
    void Post(T &&task)
    {
        boost::asio::post(io_context_, [this, task = std::forward<T>(task)]() mutable {
            boost::fibers::fiber(boost::fibers::launch::dispatch, std::allocator_arg, CustomStackAllocator{},
                                 std::forward<T>(task))
                .detach();
        });
    }

    void Stop()
    {
        work_guard_.reset();
        io_context_.stop();
        stopped_ = true;

        if (work_thread_.joinable()) {
            work_thread_.join();
        }
        for (auto &thread : additional_work_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    boost::asio::io_context &GetIOContext()
    {
        return io_context_;
    }

  private:
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<decltype(io_context_.get_executor())> work_guard_;
    std::thread work_thread_;
    std::vector<std::thread> additional_work_threads_;
    std::atomic<bool> stopped_{false};
    MemoryPool memory_pool_{30000};
};

template <typename Network = Default>
EventLoop<Network> &getEventLoop()
{
    static EventLoop<Network> event_loop;
    return event_loop;
}

template <typename F, typename Tuple, std::size_t... Is>
auto call_with_tuple(F &&f, Tuple &&t, std::index_sequence<Is...>)
{
    return f(std::get<Is>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
auto call_with_tuple(F &&f, Tuple &&t)
{
    constexpr auto size = std::tuple_size<std::decay_t<Tuple>>::value;
    return call_with_tuple(std::forward<F>(f), std::forward<Tuple>(t), std::make_index_sequence<size>{});
}

template <typename FuncSignature, typename Network, typename signal_type>
signal_type &GetSignal()
{
    static signal_type signal;
    return signal;
}

template <typename FuncSignature, typename Network = internal::Default>
struct EventDispatcher {
    template <typename Callable>
    static boost::signals2::connection subscribe(Callable &&callable)
    {
        return GetSignal<FuncSignature, Network, signal_type>().connect(std::forward<Callable>(callable));
    }

    template <typename... Args>
    static void publish(Args &&...args)
    {
        auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
        getEventLoop<Network>().Post([argsTuple = std::move(argsTuple)]() mutable {
            call_with_tuple(GetSignal<FuncSignature, Network, signal_type>(), std::move(argsTuple));
        });
    }

    using args_t = typename args_t_or_default<FuncSignature, has_args_t<FuncSignature>::value>::type;

    using signal_type = typename SignalFromTuple<args_t>::type;
};

inline boost::optional<boost::asio::deadline_timer::traits_type::time_type> &Now()
{
    static boost::optional<boost::asio::deadline_timer::traits_type::time_type> now;
    return now;
}

// This clock can be settable in unit test
struct MockableClock {
    typedef boost::asio::deadline_timer::traits_type source_traits;

  public:
    typedef source_traits::time_type time_type;
    typedef source_traits::duration_type duration_type;

    static time_type now()
    {
        if (Now()) {
            return *Now();
        }

        return chrono_to_ptime(std::chrono::system_clock::now());
    }

    // To be used in testing only

    static void set_now(time_type t = chrono_to_ptime(std::chrono::system_clock::now()))
    {
        Now() = t;
    }

    template <typename Duration>
    static void advance_time(Duration &&d)
    {
        if (!Now()) {
            return;
        }
        *Now() = add(*Now(),
                     boost::posix_time::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(d).count()));
        std::this_thread::sleep_for(std::chrono::microseconds(1100));
    }

    static time_type add(time_type t, duration_type d)
    {
        return source_traits::add(t, d);
    }
    static duration_type subtract(time_type t1, time_type t2)
    {
        return source_traits::subtract(t1, t2);
    }
    static bool less_than(time_type t1, time_type t2)
    {
        return source_traits::less_than(t1, t2);
    }

    static boost::posix_time::time_duration to_posix_duration(duration_type d)
    {
        if (!Now()) {
            return d;
        }
        return d < boost::posix_time::milliseconds(1) ? d : boost::posix_time::milliseconds(1);
    }

    static boost::posix_time::ptime chrono_to_ptime(const std::chrono::system_clock::time_point &tp)
    {
        auto duration = tp.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration - seconds);
        return boost::posix_time::from_time_t(seconds.count()) + boost::posix_time::microseconds(microseconds.count());
    }
};

}  // namespace internal

// ========================================= API ========================================= //

/**
 * @brief Attach a callable to a function signature.
 *
 * This function binds a callable (e.g., a lambda, function, or functor) to a specific function signature.
 * The callable must match the return type and argument types defined by the function signature.
 *
 * @tparam FuncSignature The function signature to attach the callable to.
 * @tparam Callable The type of the callable.
 * @param callable The callable to attach.
 *
 * @note If the callable's return type does not match the function signature's return type, a static assertion will
 * fail.
 *
 * Example:
 * @code
 * struct Addition {
 *     using args_t = std::tuple<int, int>;
 *     using return_t = int;
 * };
 *
 * dispatcher::attach<Addition>([](int a, int b) { return a + b; });
 * @endcode
 */
template <typename FuncSignature, typename Callable>
void attach(Callable &&callable)
{
    static_assert(
        std::is_same<typename internal::FunctionDispatcher<FuncSignature>::return_t,
                     typename internal::ReturnTypeFromCallable<
                         Callable, typename internal::FunctionDispatcher<FuncSignature>::args_t>::type>::value,
        "The return values of the callable is not matching the function signature");
    internal::FunctionDispatcher<FuncSignature>::template attach<Callable>(std::forward<Callable>(callable));
}

/**
 * @brief Detach the callable from a function signature.
 *
 * This function removes the callable that was previously attached to the specified function signature.
 * After detaching, any calls to the function signature will throw an exception.
 *
 * @tparam FuncSignature The function signature to detach the callable from.
 */
template <typename FuncSignature>
void detach()
{
    internal::FunctionDispatcher<FuncSignature>::detach();
}

/**
 * @brief Call a function by its signature with the provided arguments.
 *
 * This function invokes the callable previously attached to the specified function signature with the given arguments.
 * The arguments must match the types defined in the function signature.
 * This function is blocking
 *
 * @tparam FuncSignature The function signature to call.
 * @tparam Args The types of the arguments to pass to the callable.
 * @param args The arguments to pass to the callable.
 * @return The return value of the callable.
 *
 * @throws NoHandler<FuncSignature> If no callable is attached to the function signature.
 *
 * Example:
 * @code
 * struct Addition {
 *     using args_t = std::tuple<int, int>;
 *     using return_t = int;
 * };
 *
 * dispatcher::attach<Addition>([](int a, int b) { return a + b; });
 * int result = dispatcher::call<Addition>(3, 5); // result will be 8
 * @endcode
 */
template <typename FuncSignature, typename... Args>
auto call(Args &&...args)
{
    return internal::FunctionDispatcher<FuncSignature>::call(std::forward<Args>(args)...);
}

/**
 * @brief Perform an asynchronous function call by its signature with the provided arguments.
 *
 * This function invokes the callable attached to the specified function signature asynchronously.
 * The arguments must match the types defined in the function signature. The function returns a
 * `boost::fibers::future` that can be used to retrieve the result of the callable once it completes.
 *
 * @tparam FuncSignature The function signature to call.
 * @tparam Network The network type, aka wich EventLoop will handle this (default is `internal::Default`).
 * @tparam Args The types of the arguments to pass to the callable.
 * @param args The arguments to pass to the callable.
 * @return A `boost::fibers::future` containing the return value of the callable.
 *
 * @throws NoHandler<FuncSignature> If no callable is attached to the function signature.
 *
 * @note This function does not block the calling thread. The callable is executed in the context
 * of the event loop associated with the specified network.
 *
 * Example
 * @code
 * struct Addition {
 *     using args_t = std::tuple<int, int>;
 *     using return_t = int;
 * };
 *
 * dispatcher::attach<Addition>([](int a, int b) { return a + b; });
 * auto future = dispatcher::async_call<Addition>(3, 5);
 * int result = future.get(); // result will be 8
 * @endcode
 */
template <typename FuncSignature, typename Network = internal::Default, typename... Args>
auto async_call(Args &&...args)
{
    boost::fibers::promise<typename internal::FunctionDispatcher<FuncSignature>::return_t> promise;
    auto future = promise.get_future();

    using func_type = typename internal::FunctionDispatcher<FuncSignature>::func_type;

    auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
    internal::getEventLoop<Network>().Post([promise = std::move(promise), argsTuple = std::move(argsTuple)]() mutable {
        promise.set_value(
            internal::call_with_tuple(internal::GetFunction<FuncSignature, func_type>(), std::move(argsTuple)));
    });
    return future;
}

/**
 * @brief Subscribe to an event with a callable.
 *
 * This function allows you to subscribe to an event by providing a callable (e.g., a lambda, function, or functor).
 * The callable will be invoked whenever the event is published.
 *
 * @tparam FuncSignature The function signature of the event to subscribe to.
 * @tparam Network The network type, aka which event loop will handle this (default is `internal::Default`).
 * @tparam Callable The type of the callable.
 * @param callable The callable to invoke when the event is published.
 * @return A `boost::signals2::connection` object representing the subscription.
 *
 * Example:
 * @code
 * struct MyEvent {
 *     using args_t = std::tuple<int, std::string>;
 * };
 *
 * dispatcher::subscribe<MyEvent>([](int a, const std::string& b) {
 *     std::cout << "Received event with values: " << a << ", " << b << std::endl;
 * });
 * @endcode
 */
template <typename FuncSignature, typename Network = internal::Default, typename Callable>
boost::signals2::connection subscribe(Callable &&callable)
{
    return internal::EventDispatcher<FuncSignature, Network>::subscribe(std::forward<Callable>(callable));
}

/**
 * @brief Wait for an event to be published.
 *
 * This function does not block the current fiber directly. Instead, it returns a future
 * that will be fulfilled when the specified event is published. The caller can block on
 * the returned future if needed to wait for the event.
 *
 * @tparam FuncSignature The function signature of the event to wait for.
 * @tparam Network The network type (default is `internal::Default`).
 * @return A `boost::fibers::future<std::nullptr_t>` that will be fulfilled when the event is published.
 *
 * Example:
 * @code
 * struct MyEvent {
 *     using args_t = std::tuple<>;
 * };
 *
 * auto future = dispatcher::expect<MyEvent>();
 * future.wait(); // Blocks until the event is published
 * std::cout << "Event received!" << std::endl;
 * @endcode
 */
template <typename FuncSignature, typename Network = internal::Default>
boost::fibers::future<std::nullptr_t> expect()
{
    auto promise = std::make_shared<boost::fibers::promise<std::nullptr_t>>();
    auto future = promise->get_future();

    auto connection = std::make_shared<boost::signals2::connection>();
    *connection =
        internal::EventDispatcher<FuncSignature, Network>::subscribe([promise, connection](auto &&...args) mutable {
            connection->disconnect();
            promise->set_value({});
        });

    return future;
}

/**
 * @brief Wait for an event to be published and invoke a callable.
 *
 * This function does not block the current fiber directly. Instead, it returns a future
 * that will be fulfilled when the specified event is published. Additionally, the provided
 * callable is invoked with the event's arguments when the event is triggered.
 *
 * @tparam FuncSignature The function signature of the event to wait for.
 * @tparam Network The network type (default is `internal::Default`).
 * @tparam Callable The type of the callable.
 * @param callable The callable to invoke when the event is published.
 * @return A `boost::fibers::future<std::nullptr_t>` that will be fulfilled after the callable is executed.
 *
 * Example:
 * @code
 * struct MyEvent {
 *     using args_t = std::tuple<int, std::string>;
 * };
 *
 * auto future = dispatcher::expect<MyEvent>([](int a, const std::string& b) {
 *     std::cout << "Received event with values: " << a << ", " << b << std::endl;
 * });
 * future.wait(); // Blocks until the event is published
 * std::cout << "Event handling completed!" << std::endl;
 * @endcode
 */
template <typename FuncSignature, typename Network = internal::Default, typename Callable>
boost::fibers::future<std::nullptr_t> expect(Callable &&callable)
{
    auto promise = std::make_shared<boost::fibers::promise<std::nullptr_t>>();
    auto future = promise->get_future();

    auto connection = std::make_shared<boost::signals2::connection>();
    *connection = internal::EventDispatcher<FuncSignature, Network>::subscribe(
        [promise, callable = std::forward<Callable>(callable), connection](auto &&...args) mutable {
            connection->disconnect();
            callable(std::forward<decltype(args)>(args)...);
            promise->set_value({});
        });

    return future;
}

/**
 * @brief Publish an event with the specified arguments.
 *
 * This function triggers an event by its function signature and passes the provided arguments
 * to all subscribed callables. The event is handled asynchronously by the associated event loop.
 *
 * @tparam FuncSignature The function signature of the event to publish.
 * @tparam Network The network type (default is `internal::Default`).
 * @tparam Args The types of the arguments to pass to the event.
 * @param args The arguments to pass to the event.
 *
 * Example
 * @code
 * struct MyEvent {
 *     using args_t = std::tuple<int, std::string>;
 * };
 *
 * dispatcher::subscribe<MyEvent>([](int a, const std::string& b) {
 *     std::cout << "Received event with values: " << a << ", " << b << std::endl;
 * });
 *
 * dispatcher::publish<MyEvent>(42, "Hello, World!");
 * @endcode
 */
template <typename FuncSignature, typename Network = internal::Default, typename... Args>
void publish(Args &&...args)
{
    internal::EventDispatcher<FuncSignature, Network>::publish(std::forward<Args>(args)...);
}

/**
 * @brief Post a task to the event loop for asynchronous execution.
 *
 * This function schedules a task to be executed asynchronously by the event loop associated
 * with the specified network.
 *
 * @tparam Network The network type (default is `internal::Default`).
 * @tparam T The type of the task.
 * @param task The task to execute asynchronously.
 *
 * Example:
 * @code
 * dispatcher::post([] {
 *     std::cout << "Task executed asynchronously!" << std::endl;
 * });
 * @endcode
 */
template <typename Network = internal::Default, typename T>
void post(T &&task)
{
    internal::getEventLoop<Network>().Post(std::forward<T>(task));
}

/**
 * @brief Set the number of worker threads for the event loop.
 *
 * This function configures the number of worker threads in the event loop associated with the specified network.
 * Worker threads are used to process tasks asynchronously. By default, the event loop runs with a single thread.
 *
 * @tparam Network The network type (default is `internal::Default`).
 * @param thread_count The number of worker threads to set for the event loop.
 *
 * @note The number of threads must be greater than or equal to 1. If the current number of threads is already greater
 * than or equal to the specified thread count, no additional threads will be created, and the event loop configuration
 * remains unchanged.
 *
 * Example:
 * @code
 * dispatcher::set_worker_threads<WorkNetwork>(10); // Set 10 worker threads for the WorkNetwork event loop.
 * @endcode
 */
template <typename Network = internal::Default>
void set_worker_threads(int thread_count)
{
    internal::getEventLoop<Network>().SetWorkerThreadsAmount(thread_count);
}

/**
 * @brief A timer utility for scheduling tasks in the event loop.
 *
 * The `Timer` class provides functionality to schedule tasks to be executed after a specified duration
 * or repeatedly at regular intervals. It uses the event loop associated with the specified network
 * to handle the execution of tasks asynchronously.
 *
 * @tparam Network The network type (default is `internal::Default`).
 */
template <typename Network>
class Timer {
  public:
    /**
     * @brief Construct a new Timer object.
     *
     * Initializes the timer using the IO context of the event loop associated with the specified network.
     */
    Timer() : timer_(internal::getEventLoop<Network>().GetIOContext())
    {
    }

    /**
     * @brief Cancel the timer.
     *
     * Cancels any pending tasks associated with the timer. If the timer is already expired or no tasks
     * are pending, this function has no effect.
     */
    void Cancel()
    {
        timer_.cancel();
    }

    /**
     * @brief Schedule a task to be executed after a specified duration.
     *
     * This function schedules a task to be executed once after the specified duration has elapsed.
     *
     * @tparam Duration The type of the duration (e.g., `std::chrono::milliseconds`).
     * @tparam Callback The type of the callback function.
     * @param duration The duration to wait before executing the task.
     * @param callback The callback function to execute after the duration.
     *
     * Example:
     * @code
     * dispatcher::Timer<> timer;
     * timer.DoIn(std::chrono::seconds(5), [] {
     *     std::cout << "Task executed after 5 seconds!" << std::endl;
     * });
     * @endcode
     */
    template <typename Duration, typename Callback>
    void DoIn(Duration duration, Callback &&callback)
    {
        timer_.expires_from_now(
            boost::posix_time::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
        timer_.async_wait([callback = std::forward<Callback>(callback)](const boost::system::error_code &ec) mutable {
            if (ec != boost::asio::error::operation_aborted) {
                internal::getEventLoop<Network>().Post(std::forward<Callback>(callback));
            }
        });
    }

    /**
     * @brief Schedule a task to be executed repeatedly at regular intervals.
     *
     * This function schedules a task to be executed repeatedly at the specified interval.
     *
     * @tparam Duration The type of the duration (e.g., `std::chrono::milliseconds`).
     * @tparam Callback The type of the callback function.
     * @param duration The interval at which to execute the task.
     * @param callback The callback function to execute at each interval.
     *
     * Example:
     * @code
     * dispatcher::Timer<> timer;
     * timer.DoEvery(std::chrono::seconds(2), [] {
     *     std::cout << "Task executed every 2 seconds!" << std::endl;
     * });
     * @endcode
     */
    template <typename Duration, typename Callback>
    void DoEvery(Duration duration, Callback &&callback)
    {
        timer_.expires_from_now(
            boost::posix_time::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
        timer_.async_wait(
            [this, callback = std::forward<Callback>(callback), duration](const boost::system::error_code &ec) mutable {
                if (ec != boost::asio::error::operation_aborted) {
                    internal::getEventLoop<Network>().Post(callback);
                    DoEvery(duration, std::forward<Callback>(callback));
                }
            });
    }

  private:
    boost::asio::basic_deadline_timer<boost::posix_time::ptime, internal::MockableClock> timer_;
};

using DefaultTimer = Timer<internal::Default>;

}  // namespace dispatcher