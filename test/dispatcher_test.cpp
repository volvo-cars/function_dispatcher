// Copyright 2025 Volvo Car Corporation
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

#include "dispatcher_test.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <tuple>

struct Addition {
    using args_t = std::tuple<float, int>;
    using return_t = int;
};

struct Multiplication {
    using args_t = std::tuple<float, float>;
    using return_t = float;
};

struct SomeEvent {
    using parameters_t = std::tuple<bool, const std::string&>;
};

struct AnotherEvent {
    using parameters_t = std::tuple<>;
};

class ExampleTest : public dispatcher::Test {};

TEST_F(ExampleTest, ExpectingEventUnordered)
{
    DISPATCHER_EXPECT_EVENT(SomeEvent, dispatcher::_, "Hello")
        .Times(2)
        .WillRepeatedly([](bool should_print, auto message) {
            if (should_print) {
                std::cout << message << std::endl;
            }
        });
    DISPATCHER_EXPECT_EVENT(SomeEvent, false, "World").Times(0);
    DISPATCHER_EXPECT_EVENT(AnotherEvent);

    dispatcher::publish<AnotherEvent>();
    dispatcher::publish<SomeEvent>(true, "Hello");
    dispatcher::publish<SomeEvent>(false, "Hello");
}

TEST_F(ExampleTest, ExpectingEventOrdered)
{
    dispatcher::InSequence sequence;

    DISPATCHER_EXPECT_EVENT(SomeEvent, true, "Hello");
    DISPATCHER_EXPECT_EVENT(AnotherEvent).Times(2);
    DISPATCHER_EXPECT_EVENT(SomeEvent, false, "Hello");

    dispatcher::publish<SomeEvent>(true, "Hello");
    dispatcher::publish<AnotherEvent>();
    dispatcher::publish<AnotherEvent>();
    dispatcher::publish<SomeEvent>(false, "Hello");
}

TEST_F(ExampleTest, ExpectingCallUnordered)
{
    DISPATCHER_ON_CALL(Addition).WillByDefault([](auto...) { return 3; });
    DISPATCHER_EXPECT_CALL(Addition, 2, 3).Times(2).WillRepeatedly([](auto a, auto b) { return a + b; });
    DISPATCHER_EXPECT_CALL(Addition, 1, 2).Times(0);
    DISPATCHER_EXPECT_CALL(Addition, 1, 3).WillOnce([](auto...) { return 10; });
    DISPATCHER_EXPECT_CALL(Addition, 4, 3);

    EXPECT_EQ(dispatcher::call<Addition>(2, 3), 5);
    EXPECT_EQ(dispatcher::call<Addition>(1, 3), 10);
    EXPECT_EQ(dispatcher::call<Addition>(2, 3), 5);
    EXPECT_EQ(dispatcher::call<Addition>(4, 3), 0);
    EXPECT_EQ(dispatcher::call<Addition>(0, 5), 3);
}

TEST_F(ExampleTest, ExpectingCallOrdered)
{
    dispatcher::InSequence sequence;

    DISPATCHER_EXPECT_CALL(Addition, 2, 5);
    DISPATCHER_EXPECT_CALL(Addition, 1, 3).Times(2);
    DISPATCHER_EXPECT_CALL(Addition, 4, 3).Times(0);
    DISPATCHER_EXPECT_CALL(Addition, 2, 5);

    dispatcher::call<Addition>(2, 5);
    dispatcher::call<Addition>(1, 3);
    dispatcher::call<Addition>(1, 3);
    dispatcher::call<Addition>(2, 5);
}

TEST_F(ExampleTest, ExpectingCallAndEventOrdered)
{
    using dispatcher::_;

    dispatcher::InSequence sequence;

    DISPATCHER_EXPECT_CALL(Addition, _, _).WillOnce([](auto a, auto b) { return a + b; });
    DISPATCHER_EXPECT_EVENT(SomeEvent, _, _).WillOnce([](auto...) {
        EXPECT_EQ(dispatcher::call<Multiplication>(5, 5), 25);
    });
    DISPATCHER_EXPECT_CALL(Multiplication, _, _).WillOnce([](auto a, auto b) { return a * b; });

    EXPECT_EQ(dispatcher::call<Addition>(2, 5), 7);
    dispatcher::publish<SomeEvent>(false, "Hello world");
}

TEST_F(ExampleTest, TimerTest)
{
    DISPATCHER_ENABLE_MANUAL_TIME();
    dispatcher::DefaultTimer timer;
    DISPATCHER_EXPECT_EVENT(AnotherEvent);
    timer.DoIn(std::chrono::seconds{1}, [] { dispatcher::publish<AnotherEvent>(); });
    DISPATCHER_ADVANCE_TIME(std::chrono::seconds{1});
}

TEST_F(ExampleTest, RecurrentTimerTest)
{
    DISPATCHER_ENABLE_MANUAL_TIME();
    dispatcher::DefaultTimer timer;
    DISPATCHER_EXPECT_EVENT(AnotherEvent).Times(3);
    timer.DoEvery(std::chrono::seconds{1}, [] { dispatcher::publish<AnotherEvent>(); });
    DISPATCHER_ADVANCE_TIME(std::chrono::seconds{1});
    DISPATCHER_ADVANCE_TIME(std::chrono::seconds{1});
    DISPATCHER_ADVANCE_TIME(std::chrono::seconds{1});
}

struct CallWithReferences {
    using args_t = std::tuple<std::string&>;
    using return_t = bool;
};

TEST_F(ExampleTest, OnCallWithReferences)
{
    DISPATCHER_ON_CALL(CallWithReferences).WillByDefault([](auto...) { return true; });
    DISPATCHER_EXPECT_CALL(CallWithReferences, [](const auto& message) { return true; }).WillOnce([](auto...) {
        return true;
    });
    DISPATCHER_EXPECT_CALL(CallWithReferences, dispatcher::_).WillOnce([](auto...) { return true; });
    std::string a_string{"Hello"};
    dispatcher::call<CallWithReferences>(a_string);
    dispatcher::call<CallWithReferences>(a_string);
    dispatcher::call<CallWithReferences>(a_string);
}
