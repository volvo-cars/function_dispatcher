#include <iostream>
#include <boost/any.hpp>
#include <functional>
#include <boost/ref.hpp>
#include <tuple>
#include <vector>
#include <string>
#include <map>
#include <typeindex>

class EventDispatcher
{
public:
  template <typename FuncSignature, typename Func>
  void Attach(Func &&func)
  {
    return Attach_impl<FuncSignature>(std::forward<Func>(func), std::is_same<void, typename FuncSignature::return_t>{});
  }

  template <typename FuncSignature, typename... Args>
  typename FuncSignature::return_t Call(Args &&...args)
  {
    return Call_impl<FuncSignature>(std::is_same<void, typename FuncSignature::return_t>{}, std::forward<Args>(args)...);
  }

private:
  template <typename FuncSignature, typename Func>
  void Attach_impl(Func &&func, std::false_type)
  {
    callbacks[typeid(FuncSignature)] = [f = std::forward<Func>(func)](boost::any args) mutable -> boost::any
    { return call_with_args<FuncSignature>(std::forward<Func>(f), std::move(args), std::make_index_sequence<std::tuple_size<typename FuncSignature::args_t>::value>{}); };
  }

  template <typename FuncSignature, typename Func>
  void Attach_impl(Func &&func, std::true_type)
  {
    callbacks_void[typeid(FuncSignature)] = [f = std::forward<Func>(func)](boost::any args) mutable
    { call_with_args_void<FuncSignature>(std::forward<Func>(f), std::move(args), std::make_index_sequence<std::tuple_size<typename FuncSignature::args_t>::value>{}); };
  }

  template <typename FuncSignature, typename... Args>
  typename FuncSignature::return_t Call_impl(std::false_type, Args &&...args)
  {
    using args_t = std::tuple<Args...>;
    static_assert(std::is_same<typename FuncSignature::args_t, args_t>::value, "Wrong arguments");
    return boost::any_cast<typename FuncSignature::return_t>(callbacks[typeid(FuncSignature)](args_t{std::forward<Args>(args)...}));
  }

  template <typename FuncSignature, typename... Args>
  void Call_impl(std::true_type, Args &&...args)
  {
    using args_t = std::tuple<Args...>;
    static_assert(std::is_same<typename FuncSignature::args_t, args_t>::value, "Wrong arguments");
    callbacks_void[typeid(FuncSignature)](args_t{std::forward<Args>(args)...});
  }

  template <typename FuncSignature, typename Func, std::size_t... I>
  static boost::any call_with_args(Func &&func, boost::any args, std::index_sequence<I...>)
  {

    static_assert(std::is_same<
                      typename FuncSignature::return_t,
                      typename std::result_of<Func(std::tuple_element_t<I, typename FuncSignature::args_t>...)>::type>::value,
                  "Wrong return type");
    using args_t = typename FuncSignature::args_t;
    return boost::any{func(std::get<I>(boost::any_cast<args_t>(std::move(args)))...)};
  }

  template <typename FuncSignature, typename Func, std::size_t... I>
  static void call_with_args_void(Func &&func, boost::any args, std::index_sequence<I...>)
  {

    static_assert(std::is_same<
                      typename FuncSignature::return_t,
                      typename std::result_of<Func(std::tuple_element_t<I, typename FuncSignature::args_t>...)>::type>::value,
                  "Wrong return type");
    using args_t = typename FuncSignature::args_t;
    func(std::get<I>(boost::any_cast<args_t>(std::move(args)))...);
  }

  // TODO unordered map ?
  std::map<std::type_index, std::function<boost::any(boost::any)>> callbacks;
  std::map<std::type_index, std::function<void(boost::any)>> callbacks_void;
};
