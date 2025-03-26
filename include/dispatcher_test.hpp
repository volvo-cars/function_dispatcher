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

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <tuple>
#include <typeindex>
#include <vector>

#include "dispatcher.hpp"

namespace dispatcher {

namespace internal {

struct Sequence {
    int next_expected_call = 0;
    std::atomic<int> current_call_to_expect;
};

thread_local std::shared_ptr<internal::Sequence> thread_local_sequence = nullptr;

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
        : sequence_(thread_local_sequence),
          file(_file),
          line(_line),
          return_function(std::forward<ReturnFunction>(_return_function)),
          matchers(std::forward<Matcher>(_matcher)),
          expected_calls_left(_expected_calls_left)
    {
        if (sequence_ != nullptr) {
            rank_in_sequence = sequence_->next_expected_call;
            sequence_->next_expected_call += expected_calls_left;
        }
    }
    Expectation(const Expectation &) = delete;
    Expectation &operator=(const Expectation &) = delete;

    Expectation(Expectation &&other) noexcept
        : sequence_(std::move(other.sequence_)),
          file(other.file),
          line(other.line),
          return_function(std::move(other.return_function)),
          matchers(std::move(other.matchers)),
          expected_calls_left(other.expected_calls_left),
          rank_in_sequence(other.rank_in_sequence)
    {
        other.file = nullptr;
        other.line = 0;
        other.expected_calls_left = 0;
        other.rank_in_sequence = -1;
    }

    Expectation &operator=(Expectation &&other) noexcept
    {
        if (this != &other) {
            sequence_ = std::move(other.sequence_);
            file = other.file;
            line = other.line;
            return_function = std::move(other.return_function);
            matchers = std::move(other.matchers);
            expected_calls_left = other.expected_calls_left;
            rank_in_sequence = other.rank_in_sequence;

            other.file = nullptr;
            other.line = 0;
            other.expected_calls_left = 0;
            other.rank_in_sequence = -1;
        }
        return *this;
    }
    ~Expectation()
    {
        if (expected_calls_left != 0) {
            std::ostringstream oss;
            oss << "Still expecting " << expected_calls_left << " calls";
            GTEST_MESSAGE_AT_(file, line, oss.str().c_str(), ::testing::TestPartResult::kNonFatalFailure);
        }
    }

    template <typename... Args>
    bool validate(const Args &...args)
    {
        if (expected_calls_left == 0) {
            return false;
        }

        if (!call_tuple(matchers, std::make_index_sequence<tuple_size>{}, args...)) {
            return false;
        }
        if (rank_in_sequence != -1 && sequence_ != nullptr) {
            if (rank_in_sequence != sequence_->current_call_to_expect) {
                std::cout << "Expectation constructed at " << file << ":" << line
                          << " expected call out of sequence. Rank in sequence : " << rank_in_sequence
                          << " while current call to expect is : " << sequence_->current_call_to_expect << std::endl;
                return false;
            }
            sequence_->current_call_to_expect++;
        }
        return true;
    }

    template <typename... Args>
    typename FunctionDispatcher<FuncSignature>::return_t get_return_value(Args &&...args)
    {
        expected_calls_left--;
        rank_in_sequence++;
        return return_function(std::forward<Args>(args)...);
    }

    bool expected_more_calls() const
    {
        if (expected_calls_left != 0) {
            return true;
        }
        return false;
    }

    using matchers_tuple = typename ToMatcher<typename FunctionDispatcher<FuncSignature>::args_t>::type;
    matchers_tuple matchers;
    static constexpr auto tuple_size = std::tuple_size<std::decay_t<matchers_tuple>>::value;
    int expected_calls_left;
    int rank_in_sequence = -1;
    std::shared_ptr<internal::Sequence> sequence_;
    const char *file;
    int line;
    typename FunctionDispatcher<FuncSignature>::func_type return_function;
};

class ExpecterBase {
  public:
    virtual ~ExpecterBase() = default;
};

template <typename FuncSignature>
class CallExpecter : public ExpecterBase {
  public:
    CallExpecter()
    {
        ArgsFromTuple<typename FunctionDispatcher<FuncSignature>::args_t>{}.attach(*this);
    }
    CallExpecter(const CallExpecter &) = delete;
    CallExpecter &operator=(const CallExpecter &) = delete;
    CallExpecter(CallExpecter &&) = default;
    CallExpecter &operator=(CallExpecter &&) = default;

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
            throw NoHandler<FuncSignature>{};
        });
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

#define GET_CALL_EXPECTER(FuncSignature)                               \
    dynamic_cast<dispatcher::internal::CallExpecter<FuncSignature> *>( \
        expecter_container_->expecters_[typeid(FuncSignature)].get())

template <typename FuncSignature>
class EventExpecter : public ExpecterBase {
  public:
    EventExpecter()
    {
        ArgsFromTuple<typename EventDispatcher<FuncSignature>::args_t>{}.subscribe(*this);
    }
    EventExpecter(const EventExpecter &) = delete;
    EventExpecter &operator=(const EventExpecter &) = delete;
    EventExpecter(EventExpecter &&) = default;
    EventExpecter &operator=(EventExpecter &&) = default;

    template <typename... Args>
    void subscribe()
    {
        dispatcher::subscribe<FuncSignature>([this](Args &&...args) {
            for (const auto &should_not_be_published : should_not_be_published_) {
                if (call_tuple(std::get<2>(should_not_be_published),
                               std::make_index_sequence<Expectation<FuncSignature>::tuple_size>{}, args...)) {
                    GTEST_MESSAGE_AT_(std::get<0>(should_not_be_published), std::get<1>(should_not_be_published),
                                      "Event was not expected", ::testing::TestPartResult::kNonFatalFailure);
                }
            }

            for (auto &expectation : remaining_expectation_) {
                if (expectation.validate(args...)) {
                    expectation.get_return_value(std::forward<Args>(args)...);
                    break;
                }
            }
        });
    }

    void add_expectation(Expectation<FuncSignature> expectation)
    {
        remaining_expectation_.push_back(std::move(expectation));
    }

    template <typename Matchers>
    void add_should_not_call_matchers(const char *file, int line, Matchers &&matchers)
    {
        should_not_be_published_.emplace_back(file, line, std::forward<Matchers>(matchers));
    }

    void clean_expectations()
    {
        remaining_expectation_.clear();
        should_not_be_published_.clear();
    }

  private:
    std::vector<Expectation<FuncSignature>> remaining_expectation_;
    std::vector<std::tuple<const char *, int, typename Expectation<FuncSignature>::matchers_tuple>>
        should_not_be_published_;

    // Internal helper class to get Args... from the tuple
    template <typename... Args>
    struct ArgsFromTuple;

    template <typename... Args>
    struct ArgsFromTuple<std::tuple<Args...>> {
        template <typename Expecter>
        void subscribe(Expecter &expecter)
        {
            expecter.template subscribe<Args...>();
        }
    };
};

#define GET_EVENT_EXPECTER(FuncSignature)                               \
    dynamic_cast<dispatcher::internal::EventExpecter<FuncSignature> *>( \
        expecter_container_->expecters_[typeid(FuncSignature)].get())

struct ExpecterContainer {
    std::map<std::type_index, std::unique_ptr<ExpecterBase>> expecters_;
};

// Function for void return type
template <typename FuncSignature>
typename std::enable_if<std::is_void<typename FunctionDispatcher<FuncSignature>::return_t>::value>::type return_value()
{
    return;
}

// Function for non-void return type that is default constructible
template <typename FuncSignature>
typename std::enable_if<!std::is_void<typename FunctionDispatcher<FuncSignature>::return_t>::value &&
                            std::is_default_constructible<typename FunctionDispatcher<FuncSignature>::return_t>::value,
                        typename FunctionDispatcher<FuncSignature>::return_t>::type
return_value()
{
    return typename FunctionDispatcher<FuncSignature>::return_t{};
}

// Function for non-void return type that is not default constructible
template <typename FuncSignature>
typename std::enable_if<!std::is_void<typename FunctionDispatcher<FuncSignature>::return_t>::value &&
                            !std::is_default_constructible<typename FunctionDispatcher<FuncSignature>::return_t>::value,
                        typename FunctionDispatcher<FuncSignature>::return_t>::type
return_value()
{
    throw std::runtime_error("Return type is not default constructible");
}

template <typename FuncSignature>
class CallExpectationBuilder {
  public:
    template <typename... Matcher>
    CallExpectationBuilder(ExpecterContainer *expecter_container, const char *file, int line, Matcher &&...matcher)
        : expecter_container_(expecter_container),
          file_(file),
          line_(line),
          matchers_{std::forward<Matcher>(matcher)...}
    {
        if (expecter_container_->expecters_.find(typeid(FuncSignature)) == expecter_container_->expecters_.end()) {
            expecter_container_->expecters_[typeid(FuncSignature)] = std::make_unique<CallExpecter<FuncSignature>>();
        }
    }
    CallExpectationBuilder(const CallExpectationBuilder &) = delete;
    CallExpectationBuilder &operator=(const CallExpectationBuilder &) = delete;
    CallExpectationBuilder(CallExpectationBuilder &&) = delete;
    CallExpectationBuilder &operator=(CallExpectationBuilder &&) = delete;
    ~CallExpectationBuilder()
    {
        if (times_to_match_ > 0) {
            GET_CALL_EXPECTER(FuncSignature)
                ->add_expectation(Expectation<FuncSignature>{
                    file_, line_, [](auto...) { return return_value<FuncSignature>(); }, matchers_, times_to_match_});
        }
    }

    CallExpectationBuilder &Times(int times_to_match)
    {
        if (times_to_match == 0) {
            GET_CALL_EXPECTER(FuncSignature)->add_should_not_call_matchers(file_, line_, matchers_);
        }
        times_to_match_ = times_to_match;
        return *this;
    }

    template <typename Callback>
    CallExpectationBuilder &WillOnce(Callback &&callback)
    {
        GET_CALL_EXPECTER(FuncSignature)
            ->add_expectation(Expectation<FuncSignature>{file_, line_, std::forward<Callback>(callback), matchers_, 1});
        times_to_match_ = 0;
        return *this;
    }

    template <typename Callback>
    CallExpectationBuilder &WillRepeatedly(Callback &&callback)
    {
        GET_CALL_EXPECTER(FuncSignature)
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
    DefaultExpectationBuilder(ExpecterContainer *expected_container) : expecter_container_(expected_container)
    {
        if (expecter_container_->expecters_.find(typeid(FuncSignature)) == expecter_container_->expecters_.end()) {
            expecter_container_->expecters_[typeid(FuncSignature)] = std::make_unique<CallExpecter<FuncSignature>>();
        }
    }
    template <typename Callback>
    void WillByDefault(Callback &&callback)
    {
        GET_CALL_EXPECTER(FuncSignature)->set_default_behavior(std::forward<Callback>(callback));
    }
    ExpecterContainer *expecter_container_;
};

template <typename FuncSignature>
class EventExpectationBuilder {
  public:
    template <typename... Matcher>
    EventExpectationBuilder(ExpecterContainer *expecter_container, const char *file, int line, Matcher &&...matcher)
        : expecter_container_(expecter_container),
          file_(file),
          line_(line),
          matchers_{std::forward<Matcher>(matcher)...}
    {
        if (expecter_container_->expecters_.find(typeid(FuncSignature)) == expecter_container_->expecters_.end()) {
            expecter_container_->expecters_[typeid(FuncSignature)] = std::make_unique<EventExpecter<FuncSignature>>();
        }
    }
    EventExpectationBuilder(const EventExpectationBuilder &) = delete;
    EventExpectationBuilder &operator=(const EventExpectationBuilder &) = delete;
    EventExpectationBuilder(EventExpectationBuilder &&) = delete;
    EventExpectationBuilder &operator=(EventExpectationBuilder &&) = delete;
    ~EventExpectationBuilder()
    {
        if (times_to_match_ > 0) {
            GET_EVENT_EXPECTER(FuncSignature)
                ->add_expectation(
                    Expectation<FuncSignature>{file_, line_, [](auto...) { return; }, matchers_, times_to_match_});
        }
    }

    EventExpectationBuilder &Times(int times_to_match)
    {
        if (times_to_match == 0) {
            GET_EVENT_EXPECTER(FuncSignature)->add_should_not_call_matchers(file_, line_, matchers_);
        }
        times_to_match_ = times_to_match;
        return *this;
    }

    template <typename Callback>
    EventExpectationBuilder &WillOnce(Callback &&callback)
    {
        GET_EVENT_EXPECTER(FuncSignature)
            ->add_expectation(Expectation<FuncSignature>{file_, line_, std::forward<Callback>(callback), matchers_, 1});
        times_to_match_ = 0;
        return *this;
    }

    template <typename Callback>
    EventExpectationBuilder &WillRepeatedly(Callback &&callback)
    {
        GET_EVENT_EXPECTER(FuncSignature)
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

}  // namespace internal

class InSequence {
  public:
    InSequence()
    {
        internal::thread_local_sequence = std::make_shared<internal::Sequence>();
    }
    ~InSequence()
    {
        internal::thread_local_sequence.reset();
    }
};

const internal::AnythingMatcher _ = {};

class Test : public testing::Test {
  protected:
    void TearDown() override
    {
        // TODO (wait for all events to be processed, then stop the event loop. Need probably a cv
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        dispatcher::getEventLoop<dispatcher::Default>().Stop();
    }
    std::unique_ptr<internal::ExpecterContainer> expecter_container_ = std::make_unique<internal::ExpecterContainer>();
};

#define DISPATCHER_EXPECT_EVENT(FuncSignature, args...)          \
    dispatcher::internal::EventExpectationBuilder<FuncSignature> \
    {                                                            \
        expecter_container_.get(), __FILE__, __LINE__, args      \
    }

#define DISPATCHER_ENABLE_MANUAL_TIME() dispatcher::MockableClock::set_now()
#define DISPATCHER_ADVANCE_TIME(duration) dispatcher::MockableClock::advance_time(duration)

#define DISPATCHER_EXPECT_CALL(FuncSignature, args...)          \
    dispatcher::internal::CallExpectationBuilder<FuncSignature> \
    {                                                           \
        expecter_container_.get(), __FILE__, __LINE__, args     \
    }

#define DISPATCHER_ON_CALL(FuncSignature)                          \
    dispatcher::internal::DefaultExpectationBuilder<FuncSignature> \
    {                                                              \
        expecter_container_.get()                                  \
    }

#define DISPATCHER_VERIFY_AND_CLEAR_EXPECTATIONS(FuncSignature) GET_CALL_EXPECTER(FuncSignature)->clean_expectations()

}  // namespace dispatcher