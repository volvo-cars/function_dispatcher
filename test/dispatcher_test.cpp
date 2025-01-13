#include "dispatcher_test.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <tuple>

struct Addition {
    using args_t = std::tuple<float, int>;
    using return_t = int;
};

DEFINE_FUNCTION_DISPATCHER(Addition)

class MyTest : public dispatcher::Test {};

TEST_F(MyTest, a)
{
    // dispatcher::InSequence sequence;
    DISPATCHER_EXPECT_CALL(Addition, 3.0f, 1).WillOnce([](float, int) { return 5; });
    DISPATCHER_EXPECT_CALL(Addition, 1.0f, 1).WillOnce([](float, int) { return 1; });

    std::cout << dispatcher::call<Addition>(1.0f, 1) << std::endl;
    std::cout << dispatcher::call<Addition>(3.0f, 1) << std::endl;
}