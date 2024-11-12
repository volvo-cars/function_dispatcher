include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.0
)
FetchContent_MakeAvailable(benchmark)

add_executable(${PROJECT_NAME}_benchmark main.cpp)
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE benchmark::benchmark ${PROJECT_NAME})

