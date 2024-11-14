#pragma once

#include <functional>
#include <tuple>
#include <unordered_map>
#include <typeindex>

class FunctionDispatcher
{
public:
    template <typename FuncSignature, typename Func>
    void Attach(Func &&func)
    {
        Attach_impl<FuncSignature>(std::forward<Func>(func), std::make_index_sequence<std::tuple_size<typename FuncSignature::args_t>::value>{});
    }

    template <typename FuncSignature, typename... Args>
    typename FuncSignature::return_t Call(Args &&...args)
    {
        return Call_impl<FuncSignature>(std::make_index_sequence<std::tuple_size<typename FuncSignature::args_t>::value>{}, std::forward<Args>(args)...);
    }

private:
    template <typename FuncSignature, typename Func, std::size_t... I>
    void Attach_impl(Func &&func, std::index_sequence<I...>)
    {
        callbacks_[typeid(FuncSignature)] = (void *)(new std::function<typename FuncSignature::return_t(std::tuple_element_t<I, typename FuncSignature::args_t>...)>{std::forward<Func>(func)});
    }

    template <typename FuncSignature, typename... Args, std::size_t... I>
    typename FuncSignature::return_t Call_impl(std::index_sequence<I...>, Args &&...args)
    {

        return (*reinterpret_cast<std::function<typename FuncSignature::return_t(std::tuple_element_t<I, typename FuncSignature::args_t>...)> *>(callbacks_[typeid(FuncSignature)]))(std::forward<Args>(args)...);
    }

    std::unordered_map<std::type_index, void *> callbacks_;
};
