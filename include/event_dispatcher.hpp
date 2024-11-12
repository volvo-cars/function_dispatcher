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
    callbacks[typeid(FuncSignature)] = [f = std::forward<Func>(func)](boost::any args) mutable -> boost::any
    { return call_with_args<FuncSignature>(std::forward<Func>(f), std::move(args), std::make_index_sequence<std::tuple_size<typename FuncSignature::args>::value>{}); };
  }

  template <typename FuncSignature, typename... Args>
  typename FuncSignature::return_type Call(Args &&...args)
  {
    using args_t = std::tuple<Args...>;
    static_assert(std::is_same<typename FuncSignature::args, args_t>::value, "Wrong arguments");
    return boost::any_cast<typename FuncSignature::return_type>(callbacks[typeid(FuncSignature)](args_t{std::forward<Args>(args)...}));
  }

private:
  template <typename FuncSignature, typename Func, std::size_t... I>
  static boost::any call_with_args(Func &&func, boost::any args, std::index_sequence<I...>)
  {

    static_assert(std::is_same<
                      typename FuncSignature::return_type,
                      typename std::result_of<Func(std::tuple_element_t<I, typename FuncSignature::args>...)>::type>::value,
                  "Wrong return type");
    using args_t = typename FuncSignature::args;
    return boost::any{func(std::get<I>(boost::any_cast<args_t>(std::move(args)))...)};
  }

  // TODO unordered map ?
  std::map<std::type_index, std::function<boost::any(boost::any)>> callbacks;
};
