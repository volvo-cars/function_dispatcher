add_executable(${PROJECT_NAME}_normal_server normal_server.cpp)

find_package(Boost
  1.83.0
  QUIET
  COMPONENTS
  asio
)

target_link_libraries(${PROJECT_NAME}_normal_server PRIVATE Boost::asio)

