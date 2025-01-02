#include "dispatcher_test.hpp"

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
    DISPATCHER_EXPECT_CALL(Addition, 1.0f, 1).WillOnce([](float, int) { return 1; });
    DISPATCHER_ON_CALL(Addition).WillByDefault([](float, int) { return 2; });

    std::cout << dispatcher::call<Addition>(1.0f, 1) << std::endl;
    DISPATCHER_VERIFY_AND_CLEAR_EXPECTATIONS(Addition);
    DISPATCHER_EXPECT_CALL(Addition, 1.0f, 1).Times(0);
    std::cout << dispatcher::call<Addition>(1.0f, 1) << std::endl;
}