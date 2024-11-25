#pragma once

#include <functional>
#include <tuple>
#include <vector>
#include <type_traits>

namespace dispatcher
{
    template <typename ReturnType, typename Tuple>
    struct FunctionFromTuple;

    template <typename ReturnType, typename... Args>
    struct FunctionFromTuple<ReturnType, std::tuple<Args...>>
    {
        using type = std::function<ReturnType(Args...)>;
    };

    template <typename...>
    using void_t = void;

    template <typename FuncSignature, typename = void>
    struct has_return_t : std::false_type
    {
    };

    template <typename FuncSignature>
    struct has_return_t<FuncSignature, void_t<typename FuncSignature::return_t>> : std::true_type
    {
    };

    template <typename FuncSignature, bool HasReturnT>
    struct return_t_or_default
    {
        using type = void;
    };

    template <typename FuncSignature>
    struct return_t_or_default<FuncSignature, true>
    {
        using type = typename FuncSignature::return_t;
    };

    template <typename FuncSignature, typename = void>
    struct has_args_t : std::false_type
    {
    };

    template <typename FuncSignature>
    struct has_args_t<FuncSignature, void_t<typename FuncSignature::args_t>> : std::true_type
    {
    };

    template <typename FuncSignature, bool HasReturnT>
    struct args_t_or_default
    {
        using type = std::tuple<>;
    };

    template <typename FuncSignature>
    struct args_t_or_default<FuncSignature, true>
    {
        using type = typename FuncSignature::args_t;
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

        using return_t = typename return_t_or_default<FuncSignature, has_return_t<FuncSignature>::value>::type;
        using args_t = typename args_t_or_default<FuncSignature, has_args_t<FuncSignature>::value>::type;

        using func_type = typename FunctionFromTuple<return_t, args_t>::type;

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

        using args_t = typename args_t_or_default<FuncSignature, has_args_t<FuncSignature>::value>::type;

        using func_type = typename FunctionFromTuple<void, args_t>::type;

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