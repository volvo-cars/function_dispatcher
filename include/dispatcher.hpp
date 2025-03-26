#pragma once

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

// ========================================= Function dispatcher ========================================= //

template <typename FuncSignature>
class NoHandler : public std::exception {
  public:
    const char *what() const noexcept override
    {
        return "Function was called but no handler was attached";
    }
};

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

template <typename FuncSignature, typename Callable>
void attach(Callable &&callable)
{
    static_assert(
        std::is_same<
            typename FunctionDispatcher<FuncSignature>::return_t,
            typename ReturnTypeFromCallable<Callable, typename FunctionDispatcher<FuncSignature>::args_t>::type>::value,
        "The return values of the callable is not matching the function signature");
    FunctionDispatcher<FuncSignature>::template attach<Callable>(std::forward<Callable>(callable));
}

template <typename FuncSignature>
void detach()
{
    FunctionDispatcher<FuncSignature>::detach();
}

template <typename FuncSignature, typename... Args>
auto call(Args &&...args)
{
    return FunctionDispatcher<FuncSignature>::call(std::forward<Args>(args)...);
}

// ========================================= Event loop ========================================= //

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
        std::unique_lock<std::mutex> ul{m_};
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
        std::unique_lock<std::mutex> ul{m_};
        allocated_blocks_.push(memory_block);
    }

    std::size_t get_size()
    {
        return size_;
    }

  private:
    std::size_t size_;
    std::queue<void *> allocated_blocks_;
    std::mutex m_;
};

class CustomStackAllocator {
  public:
    CustomStackAllocator(MemoryPool *memory_pool) : memory_pool_(memory_pool)
    {
    }

    boost::context::stack_context allocate()
    {
        boost::context::stack_context sctx;
        sctx.size = memory_pool_->get_size();
        sctx.sp = static_cast<char *>(memory_pool_->allocate()) + sctx.size;
        return sctx;
    }

    void deallocate(boost::context::stack_context &sctx) BOOST_NOEXCEPT_OR_NOTHROW
    {
        BOOST_ASSERT(sctx.sp);

        void *vp = static_cast<char *>(sctx.sp) - sctx.size;
        memory_pool_->free(vp);
    }

  private:
    MemoryPool *memory_pool_;
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
            boost::fibers::fiber(boost::fibers::launch::dispatch, std::allocator_arg,
                                 CustomStackAllocator{&memory_pool_}, std::forward<T>(task))
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

// ========================================= Event dispatcher ========================================= //

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

template <typename FuncSignature, typename Network = Default, typename... Args>
auto async_call(Args &&...args)
{
    boost::fibers::promise<typename FunctionDispatcher<FuncSignature>::return_t> promise;
    auto future = promise.get_future();

    using func_type = typename FunctionDispatcher<FuncSignature>::func_type;

    auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
    getEventLoop<Network>().Post([promise = std::move(promise), argsTuple = std::move(argsTuple)]() mutable {
        promise.set_value(call_with_tuple(GetFunction<FuncSignature, func_type>(), std::move(argsTuple)));
    });
    return future;
}

template <typename FuncSignature, typename Network = Default>
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

template <typename FuncSignature, typename Network = Default, typename Callable>
boost::signals2::connection subscribe(Callable &&callable)
{
    return EventDispatcher<FuncSignature, Network>::template subscribe(std::forward<Callable>(callable));
}

template <typename FuncSignature, typename Network = Default, typename... Args>
void publish(Args &&...args)
{
    EventDispatcher<FuncSignature, Network>::publish(std::forward<Args>(args)...);
}

template <typename Network = Default, typename T>
void post(T &&task)
{
    getEventLoop<Network>().Post(std::forward<T>(task));
}

// ========================================= Timer ========================================= //

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

template <typename Network = Default>
class Timer {
  public:
    Timer() : timer_(getEventLoop<Network>().GetIOContext())
    {
    }
    void Cancel()
    {
        timer_.cancel();
    }

    template <typename Duration, typename Callback>
    void DoIn(Duration duration, Callback &&callback)
    {
        timer_.expires_from_now(
            boost::posix_time::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
        timer_.async_wait([callback = std::forward<Callback>(callback)](const boost::system::error_code &ec) mutable {
            if (ec != boost::asio::error::operation_aborted) {
                getEventLoop<Network>().Post(std::forward<Callback>(callback));
            }
        });
    }

    template <typename Duration, typename Callback>
    void DoEvery(Duration duration, Callback &&callback)
    {
        timer_.expires_from_now(
            boost::posix_time::milliseconds(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
        timer_.async_wait(
            [this, callback = std::forward<Callback>(callback), duration](const boost::system::error_code &ec) mutable {
                if (ec != boost::asio::error::operation_aborted) {
                    getEventLoop<Network>().Post(callback);
                    DoEvery(duration, std::forward<Callback>(callback));
                }
            });
    }

  private:
    boost::asio::basic_deadline_timer<boost::posix_time::ptime, MockableClock> timer_;
};

using DefaultTimer = Timer<Default>;

}  // namespace dispatcher