#pragma once

#include <functional>
#include <tuple>
#include <unordered_map>
#include <typeindex>
#include <iostream>

class function_dispatcher_bad_function_call : public std::bad_function_call
{
public:
    explicit function_dispatcher_bad_function_call(const char *message) : message_(message), std::bad_function_call() {}
    const char *what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

class FunctionDispatcher
{
public:
    FunctionDispatcher() = default;
    FunctionDispatcher &operator=(FunctionDispatcher &&other)
    {
        callbacks_ = std::move(other.callbacks_);
        return *this;
    }
    FunctionDispatcher(FunctionDispatcher &&other)
    {
        callbacks_ = std::move(other.callbacks_);
    }
    FunctionDispatcher &operator=(const FunctionDispatcher &other) = delete;
    FunctionDispatcher(const FunctionDispatcher &other) = delete;
    ~FunctionDispatcher()
    {
        for (auto &ptr : callbacks_)
        {
            // This is ugly UB, but prevents leaks
            delete reinterpret_cast<std::function<void(void)> *>(ptr.second);
        }
    }

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
        auto previous_callback = callbacks_.find(typeid(FuncSignature));
        if (previous_callback != callbacks_.end())
        {
            delete reinterpret_cast<std::function<typename FuncSignature::return_t(std::tuple_element_t<I, typename FuncSignature::args_t>...)> *>(previous_callback->second);
        }
        callbacks_[typeid(FuncSignature)] = (void *)(new std::function<typename FuncSignature::return_t(std::tuple_element_t<I, typename FuncSignature::args_t>...)>{std::forward<Func>(func)});
    }

    template <typename FuncSignature, typename... Args, std::size_t... I>
    typename FuncSignature::return_t Call_impl(std::index_sequence<I...>, Args &&...args)
    {
        try
        {
            return (*reinterpret_cast<std::function<typename FuncSignature::return_t(std::tuple_element_t<I, typename FuncSignature::args_t>...)> *>(callbacks_.at(typeid(FuncSignature))))(std::forward<Args>(args)...);
        }
        catch (const std::out_of_range &)
        {
            throw function_dispatcher_bad_function_call(typeid(FuncSignature).name());
        }
    }

    std::unordered_map<std::type_index, void *> callbacks_;
};
