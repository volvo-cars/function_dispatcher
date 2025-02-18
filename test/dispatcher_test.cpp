// #include "dispatcher_test.hpp"

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
    using args_t = std::tuple<bool, const std::string&>;
};

struct AnotherEvent {
    using args_t = std::tuple<>;
};

// class ExampleTest : public dispatcher::Test {};

// TEST_F(ExampleTest, ExpectingEventUnordered)
// {
//     DISPATCHER_EXPECT_EVENT(SomeEvent, dispatcher::_, "Hello")
//         .Times(2)
//         .WillRepeatedly([](bool should_print, auto message) {
//             if (should_print) {
//                 std::cout << message << std::endl;
//             }
//         });
//     DISPATCHER_EXPECT_EVENT(SomeEvent, false, "World").Times(0);
//     DISPATCHER_EXPECT_EVENT(AnotherEvent);

//     dispatcher::publish<AnotherEvent>();
//     dispatcher::publish<SomeEvent>(true, "Hello");
//     dispatcher::publish<SomeEvent>(false, "Hello");
// }

// TEST_F(ExampleTest, ExpectingEventOrdered)
// {
//     dispatcher::InSequence sequence;

//     DISPATCHER_EXPECT_EVENT(SomeEvent, true, "Hello");
//     DISPATCHER_EXPECT_EVENT(AnotherEvent).Times(2);
//     DISPATCHER_EXPECT_EVENT(SomeEvent, false, "Hello");

//     dispatcher::publish<SomeEvent>(true, "Hello");
//     dispatcher::publish<AnotherEvent>();
//     dispatcher::publish<AnotherEvent>();
//     dispatcher::publish<SomeEvent>(false, "Hello");
// }

// TEST_F(ExampleTest, ExpectingCallUnordered)
// {
//     DISPATCHER_ON_CALL(Addition).WillByDefault([](auto...) { return 3; });
//     DISPATCHER_EXPECT_CALL(Addition, 2, 3).Times(2).WillRepeatedly([](auto a, auto b) { return a + b; });
//     DISPATCHER_EXPECT_CALL(Addition, 1, 2).Times(0);
//     DISPATCHER_EXPECT_CALL(Addition, 1, 3).WillOnce([](auto...) { return 10; });
//     DISPATCHER_EXPECT_CALL(Addition, 4, 3);

//     EXPECT_EQ(dispatcher::call<Addition>(2, 3), 5);
//     EXPECT_EQ(dispatcher::call<Addition>(1, 3), 10);
//     EXPECT_EQ(dispatcher::call<Addition>(2, 3), 5);
//     EXPECT_EQ(dispatcher::call<Addition>(4, 3), 0);
//     EXPECT_EQ(dispatcher::call<Addition>(0, 5), 3);
// }

// TEST_F(ExampleTest, ExpectingCallOrdered)
// {
//     dispatcher::InSequence sequence;

//     DISPATCHER_EXPECT_CALL(Addition, 2, 5);
//     DISPATCHER_EXPECT_CALL(Addition, 1, 3).Times(2);
//     DISPATCHER_EXPECT_CALL(Addition, 4, 3).Times(0);
//     DISPATCHER_EXPECT_CALL(Addition, 2, 5);

//     dispatcher::call<Addition>(2, 5);
//     dispatcher::call<Addition>(1, 3);
//     dispatcher::call<Addition>(1, 3);
//     dispatcher::call<Addition>(2, 5);
// }

// TEST_F(ExampleTest, ExpectingCallAndEventOrdered)
// {
//     using dispatcher::_;

//     dispatcher::InSequence sequence;

//     DISPATCHER_EXPECT_CALL(Addition, _, _).WillOnce([](auto a, auto b) { return a + b; });
//     DISPATCHER_EXPECT_EVENT(SomeEvent, _, _).WillOnce([](auto...) {
//         EXPECT_EQ(dispatcher::call<Multiplication>(5, 5), 25);
//     });
//     DISPATCHER_EXPECT_CALL(Multiplication, _, _).WillOnce([](auto a, auto b) { return a * b; });

//     EXPECT_EQ(dispatcher::call<Addition>(2, 5), 7);
//     dispatcher::publish<SomeEvent>(false, "Hello world");
// }

// TEST_F(ExampleTest, TimerTest)
// {
//     DISPATCHER_ENABLE_MANUAL_TIME();
//     dispatcher::DefaultTimer timer;
//     DISPATCHER_EXPECT_EVENT(AnotherEvent);
//     timer.DoIn(std::chrono::seconds{1}, [] { dispatcher::publish<AnotherEvent>(); });
//     DISPATCHER_ADVANCE_TIME(std::chrono::seconds{1});
// }

// TEST_F(ExampleTest, RecurrentTimerTest)
// {
//     DISPATCHER_ENABLE_MANUAL_TIME();
//     dispatcher::DefaultTimer timer;
//     DISPATCHER_EXPECT_EVENT(AnotherEvent).Times(3);
//     timer.DoEvery(std::chrono::seconds{1}, [] { dispatcher::publish<AnotherEvent>(); });
//     DISPATCHER_ADVANCE_TIME(std::chrono::seconds{1});
//     DISPATCHER_ADVANCE_TIME(std::chrono::seconds{1});
//     DISPATCHER_ADVANCE_TIME(std::chrono::seconds{1});
// }
