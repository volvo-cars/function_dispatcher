cmake_minimum_required(VERSION 3.24)

project(function-dispatcher VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 14)

option(${PROJECT_NAME}_example "Build examples" ON)
option(${PROJECT_NAME}_benchmark "Build benchmark" ON)
option(${PROJECT_NAME}_test "Build test" ON)

include(FetchContent)

find_package(Boost
  1.83.0
  QUIET
  COMPONENTS
  asio
  signals2
  fiber
)

if(NOT Boost_FOUND)
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
    GIT_TAG boost-1.83.0
    OVERRIDE_FIND_PACKAGE
  )
  FetchContent_MakeAvailable(Boost)

  find_package(Boost
    1.83.0
    REQUIRED
    COMPONENTS
    asio
    signals2
    fiber
  )
else()
  message(STATUS "Using installed Boost: ${Boost_DIR}")
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE FILE_SET public_headers TYPE HEADERS
  BASE_DIRS include
  FILES include/${PROJECT_NAME}.hpp)

include(GNUInstallDirs)
target_include_directories(
  ${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${PROJECT_NAME} INTERFACE Boost::signals2 Boost::asio Boost::fiber)

if(${PROJECT_NAME}_example)
  add_subdirectory(example)
endif()

if(${PROJECT_NAME}_benchmark)
  add_subdirectory(benchmark)
endif()

if(${PROJECT_NAME}_test)
  enable_testing()
  add_subdirectory(test)
endif()
