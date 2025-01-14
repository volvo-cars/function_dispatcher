enable_testing()
add_executable(${PROJECT_NAME}_test dispatcher_test.cpp)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.10.0
)
FetchContent_MakeAvailable(googletest)
target_link_libraries(${PROJECT_NAME}_test gtest gtest_main gmock ${PROJECT_NAME})

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test TEST_PREFIX ${PROJECT_NAME}_test)
