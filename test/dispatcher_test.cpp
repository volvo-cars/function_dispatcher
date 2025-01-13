#include "dispatcher_test.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <tuple>

struct Addition {
    using args_t = std::tuple<float, int>;
    using return_t = int;
};

struct RandomEvent {
    using args_t = std::tuple<bool>;
};

struct RandomEventBis {
    using args_t = std::tuple<>;
};

DEFINE_FUNCTION_DISPATCHER(Addition)
DEFINE_EVENT_DISPATCHER(RandomEvent)
DEFINE_EVENT_DISPATCHER(RandomEventBis)

class MyTest : public dispatcher::Test {};

TEST_F(MyTest, a)
{
    dispatcher::InSequence sequence;
    DISPATCHER_EXPECT_EVENT(RandomEvent, true);
    DISPATCHER_EXPECT_EVENT(RandomEventBis);

    dispatcher::publish<RandomEvent>(true);
    dispatcher::publish<RandomEventBis>();
}