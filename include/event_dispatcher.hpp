#include <iostream>
#include <boost/any.hpp>
#include <functional>
#include <boost/ref.hpp>
#include <tuple>
#include <vector>
#include <string>
#include <map>
#include <typeindex>

class event_dispatcher
{
public:
  template <typename FuncSignature, typename Func>
  void Attach(Func &&func)
  {
    callbacks[typeid(FuncSignature)] = [f = std::forward<Func>(func)](std::vector<boost::any> args) mutable -> boost::any
    { return call_with_args<FuncSignature>(std::forward<Func>(f), std::move(args), std::make_index_sequence<std::tuple_size<typename FuncSignature::args>::value>{}); };
  }

  template <typename FuncSignature, typename... Args>
  typename FuncSignature::return_type Call(Args &&...args)
  {
    using args_t = std::tuple<Args...>;
    static_assert(std::is_same<typename FuncSignature::args, args_t>::value, "Wrong arguments");

    auto vec = to_any_vector(std::forward<Args>(args)...);
    return boost::any_cast<typename FuncSignature::return_type>(callbacks[typeid(FuncSignature)](std::move(vec)));
  }

private:
  template <typename FuncSignature, typename Func, std::size_t... I>
  static boost::any call_with_args(Func &&func, std::vector<boost::any> args, std::index_sequence<I...>)
  {

    static_assert(std::is_same<
                      typename FuncSignature::return_type,
                      typename std::result_of<Func(std::tuple_element_t<I, typename FuncSignature::args>...)>::type>::value,
                  "Wrong return type");
    return boost::any{func(boost::any_cast<std::tuple_element_t<I, typename FuncSignature::args> &&>(args[I])...)};
  }

  template <typename... Args>
  static std::vector<boost::any> to_any_vector(Args &&...args)
  {
    std::vector<boost::any> result;
    to_any_vector_impl(result, std::forward<Args>(args)...);
    return result;
  }

  static void to_any_vector_impl(std::vector<boost::any> &)
  {
    // No action needed for an empty argument pack, recursion ends here.
  }

  // Recursive function to handle non-empty argument packs
  template <typename T, typename... Args>
  static void to_any_vector_impl(std::vector<boost::any> &result, T &&first, Args &&...args)
  {

    result.emplace_back(std::forward<T>(first));             // Emplace the first argument
    to_any_vector_impl(result, std::forward<Args>(args)...); // Recurse with the remaining arguments
  }

  // TODO unordered map ?
  std::map<std::type_index, std::function<boost::any(std::vector<boost::any>)>> callbacks;
};
