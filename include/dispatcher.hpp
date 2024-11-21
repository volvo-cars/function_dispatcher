#pragma once

#include <functional>
#include <tuple>

namespace dispatcher
{
    template <typename ReturnType, typename Tuple>
    struct FunctionFromTuple;

    template <typename ReturnType, typename... Args>
    struct FunctionFromTuple<ReturnType, std::tuple<Args...>>
    {
        using type = std::function<ReturnType(Args...)>;
    };

    // ========================================= Function dispatcher ========================================= //

    template <typename FuncSignature>
    struct FunctionDispatcher
    {
        template <typename Callable>
        static void attach(Callable &&callable)
        {
            FunctionDispatcher::function = callable;
        }

        static void detach()
        {
            FunctionDispatcher::function = nullptr;
        }

        template <typename... Args>
        static auto call(Args &&...args) { return FunctionDispatcher::function(std::forward<Args>(args)...); }

        using func_type = typename FunctionFromTuple<
            typename FuncSignature::return_t,
            typename FuncSignature::args_t>::type;

        static inline func_type function;
    };

    template <typename FuncSignature, typename Callable>
    void attach(Callable &&callable)
    {
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

    // ========================================= Event dispatcher ========================================= //

    template <typename FuncSignature>
    struct EventSubscription
    {
    };

    template <typename FuncSignature>
    struct EventDispatcher
    {
        template <typename Callable>
        static void subscribe(Callable &&callable)
        {
            EventDispatcher::functions.push_back(std::forward<Callable>(callable));
        }

        template <typename... Args>
        static void publish(Args &&...args)
        {
            for (const auto &function : EventDispatcher::functions)
            {
                function(std::forward<Args>(args)...);
            }
        }

        using func_type = typename FunctionFromTuple<
            void,
            typename FuncSignature::args_t>::type;

        static inline std::vector<func_type> functions;
    };

    template <typename FuncSignature, typename Callable>
    void subscribe(Callable &&callable)
    {
        EventDispatcher<FuncSignature>::template subscribe(std::forward<Callable>(callable));
    }

    template <typename FuncSignature, typename... Args>
    void publish(Args &&...args)
    {
        return EventDispatcher<FuncSignature>::publish(std::forward<Args>(args)...);
    }
}