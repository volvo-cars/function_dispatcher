#pragma once

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <tuple>
#include <typeindex>
#include <vector>

#include "dispatcher.hpp"

namespace dispatcher {

namespace details {

struct AnythingMatcher {
    template <typename T>
    operator std::function<bool(const T &)>() const
    {
        return [](const T &) -> bool { return true; };
    }
};

template <typename T>
struct Matcher {
    Matcher(const T &t) : matcher{[=](const T &other) { return other == t; }}
    {
    }
    Matcher(std::function<bool(const T &)> _matcher) : matcher(std::move(_matcher))
    {
    }

    std::function<bool(const T &)> matcher;
};

template <typename T>
struct ToMatcher {
    using type = Matcher<T>;
};

template <typename... Args>
struct ToMatcher<std::tuple<Args...>> {
    using type = std::tuple<Matcher<Args>...>;
};

// Helper to invoke tuple elements with corresponding arguments
template <typename Tuple, typename... Args, std::size_t... Is>
bool call_tuple(const Tuple &tuple, std::index_sequence<Is...>, const Args &...args)
{
    bool results[] = {std::get<Is>(tuple).matcher(args)...};
    for (bool result : results) {
        if (!result)
            return false;
    }
    return true;
}

template <typename FuncSignature>
struct Expectation {
    template <typename ReturnFunction, typename Matcher>
    Expectation(const char *_file, int _line, ReturnFunction &&_return_function, Matcher &&_matcher,
                int _expected_calls_left)
        : file(_file),
          line(_line),
          return_function(std::forward<ReturnFunction>(_return_function)),
          matchers(std::forward<Matcher>(_matcher)),
          expected_calls_left(_expected_calls_left)
    {
    }

    template <typename... Args>
    bool validate(const Args &...args)
    {
        if (expected_calls_left == 0) {
            return false;
        }
        return call_tuple(matchers, std::make_index_sequence<tuple_size>{}, args...);
    }

    template <typename... Args>
    typename FunctionDispatcher<FuncSignature>::return_t get_return_value(Args &&...args)
    {
        expected_calls_left--;
        return return_function(std::forward<Args>(args)...);
    }

    bool expected_more_calls() const
    {
        if (expected_calls_left != 0) {
            std::cout << "Expectation constructed at " << file << ":" << line << " expected " << expected_calls_left
                      << " more calls" << std::endl;
            return true;
        }
        return false;
    }

    using matchers_tuple = typename ToMatcher<typename FunctionDispatcher<FuncSignature>::args_t>::type;
    matchers_tuple matchers;
    static constexpr auto tuple_size = std::tuple_size<std::decay_t<matchers_tuple>>::value;
    int expected_calls_left;
    typename FunctionDispatcher<FuncSignature>::func_type return_function;
    const char *file;
    int line;
};

class ExpecterBase {
  public:
    virtual ~ExpecterBase() = default;
};

template <typename FuncSignature>
class Expecter : public ExpecterBase {
  public:
    Expecter()
    {
        ArgsFromTuple<typename FuncSignature::args_t>{}.attach(*this);
    }
    Expecter(const Expecter &) = delete;
    Expecter &operator=(const Expecter &) = delete;
    Expecter(Expecter &&) = default;
    Expecter &operator=(Expecter &&) = default;
    ~Expecter()
    {
        clean_expectations();
    }

    template <typename... Args>
    void attach()
    {
        dispatcher::attach<FuncSignature>([this](Args &&...args) {
            for (const auto &should_not_be_called : should_not_be_called_) {
                if (call_tuple(std::get<2>(should_not_be_called),
                               std::make_index_sequence<Expectation<FuncSignature>::tuple_size>{}, args...)) {
                    GTEST_MESSAGE_AT_(std::get<0>(should_not_be_called), std::get<1>(should_not_be_called),
                                      "Unexpected call", ::testing::TestPartResult::kNonFatalFailure);
                }
            }

            for (auto &expectation : remaining_expectation_) {
                if (expectation.validate(args...)) {
                    return expectation.get_return_value(std::forward<Args>(args)...);
                }
            }
            if (default_behavior.has_value()) {
                std::cout << typeid(FuncSignature).name() << " has no expected calls, default behavior called"
                          << std::endl;
                return default_behavior.get()(std::forward<Args>(args)...);
            }
            throw std::bad_function_call();
        });
    }

    void validate() const
    {
        bool still_expecting_calls = false;
        for (const auto &expectation : remaining_expectation_) {
            if (expectation.expected_more_calls()) {
                still_expecting_calls = true;
            }
        }
        EXPECT_FALSE(still_expecting_calls);
    }

    void add_expectation(Expectation<FuncSignature> expectation)
    {
        remaining_expectation_.push_back(std::move(expectation));
    }

    template <typename Callback>
    void set_default_behavior(Callback &&callback)
    {
        default_behavior.emplace(std::forward<Callback>(callback));
    }

    template <typename Matchers>
    void add_should_not_call_matchers(const char *file, int line, Matchers &&matchers)
    {
        should_not_be_called_.emplace_back(file, line, std::forward<Matchers>(matchers));
    }

    void clean_expectations()
    {
        validate();
        remaining_expectation_.clear();
        should_not_be_called_.clear();
    }

  private:
    std::vector<Expectation<FuncSignature>> remaining_expectation_;
    boost::optional<typename FunctionDispatcher<FuncSignature>::func_type> default_behavior;
    std::vector<std::tuple<const char *, int, typename Expectation<FuncSignature>::matchers_tuple>>
        should_not_be_called_;

    // Internal helper class to get Args... from the tuple
    template <typename... Args>
    struct ArgsFromTuple;

    template <typename... Args>
    struct ArgsFromTuple<std::tuple<Args...>> {
        template <typename Expecter>
        void attach(Expecter &expecter)
        {
            expecter.template attach<Args...>();
        }
    };
};

#define GET_EXPECTER(FuncSignature)                               \
    dynamic_cast<dispatcher::details::Expecter<FuncSignature> *>( \
        expecter_container_->expecters_[typeid(FuncSignature)].get())

struct ExpecterContainer {
    std::map<std::type_index, std::unique_ptr<ExpecterBase>> expecters_;
};

template <typename FuncSignature>
class ExpectationBuilder {
  public:
    template <typename... Matcher>
    ExpectationBuilder(ExpecterContainer *expecter_container, const char *file, int line, Matcher &&...matcher)
        : expecter_container_(expecter_container),
          file_(file),
          line_(line),
          matchers_{std::forward<Matcher>(matcher)...}
    {
        if (expecter_container_->expecters_.find(typeid(FuncSignature)) == expecter_container_->expecters_.end()) {
            expecter_container_->expecters_[typeid(FuncSignature)] = std::make_unique<Expecter<FuncSignature>>();
        }
    }
    ExpectationBuilder(const ExpectationBuilder &) = delete;
    ExpectationBuilder &operator=(const ExpectationBuilder &) = delete;
    ExpectationBuilder(ExpectationBuilder &&) = delete;
    ExpectationBuilder &operator=(ExpectationBuilder &&) = delete;
    ~ExpectationBuilder()
    {
        if (times_to_match_ > 0) {
            GET_EXPECTER(FuncSignature)
                ->add_expectation(Expectation<FuncSignature>{
                    file_, line_, [](auto...) { return typename FunctionDispatcher<FuncSignature>::return_t{}; },
                    matchers_, times_to_match_});
        }
    }

    ExpectationBuilder &Times(int times_to_match)
    {
        if (times_to_match == 0) {
            GET_EXPECTER(FuncSignature)->add_should_not_call_matchers(file_, line_, matchers_);
        }
        times_to_match_ = times_to_match;
        return *this;
    }

    template <typename Callback>
    ExpectationBuilder &WillOnce(Callback &&callback)
    {
        GET_EXPECTER(FuncSignature)
            ->add_expectation(Expectation<FuncSignature>{file_, line_, std::forward<Callback>(callback), matchers_, 1});
        times_to_match_ = 0;
        return *this;
    }

    template <typename Callback>
    ExpectationBuilder &WillRepeatedly(Callback &&callback)
    {
        GET_EXPECTER(FuncSignature)
            ->add_expectation(
                Expectation<FuncSignature>{file_, line_, std::forward<Callback>(callback), matchers_, times_to_match_});
        times_to_match_ = 0;
        return *this;
    }

  private:
    typename Expectation<FuncSignature>::matchers_tuple matchers_;
    int times_to_match_ = 1;
    ExpecterContainer *expecter_container_;
    const char *file_;
    int line_;
};

template <typename FuncSignature>
struct DefaultExpectationBuilder {
    template <typename Callback>
    void WillByDefault(Callback &&callback)
    {
        GET_EXPECTER(FuncSignature)->set_default_behavior(std::forward<Callback>(callback));
    }
    ExpecterContainer *expecter_container_;
};

}  // namespace details

const details::AnythingMatcher _ = {};

class Test : public testing::Test {
  protected:
    std::unique_ptr<details::ExpecterContainer> expecter_container_ = std::make_unique<details::ExpecterContainer>();
};

#define DISPATCHER_EXPECT_CALL(FuncSignature, args...)      \
    dispatcher::details::ExpectationBuilder<FuncSignature>  \
    {                                                       \
        expecter_container_.get(), __FILE__, __LINE__, args \
    }

#define DISPATCHER_ON_CALL(FuncSignature)                         \
    dispatcher::details::DefaultExpectationBuilder<FuncSignature> \
    {                                                             \
        expecter_container_.get()                                 \
    }

#define DISPATCHER_VERIFY_AND_CLEAR_EXPECTATIONS(FuncSignature) GET_EXPECTER(FuncSignature)->clean_expectations()

}  // namespace dispatcher
