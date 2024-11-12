cmake_minimum_required(VERSION 3.24)

project(event_dispatcher VERSION 0.1.0) 
set(CMAKE_CXX_STANDARD 14)
include(cmake/dependencies.cmake)

add_library(event_dispatcher INTERFACE)
target_sources(event_dispatcher INTERFACE FILE_SET public_headers TYPE HEADERS 
  BASE_DIRS include
  FILES include/event_dispatcher.hpp)

include(GNUInstallDirs)
target_include_directories(
	event_dispatcher INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

find_package(Boost REQUIRED CONFIG)
target_link_libraries(event_dispatcher INTERFACE Boost::any)
add_subdirectory(example)

# install(TARGETS event_dispatcher EXPORT event_dispatcher_targets FILE_SET public_headers)
# install(EXPORT event_dispatcher_targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/event_dispatcher)

# include(CMakePackageConfigHelpers)

# set(SYSCONFIG_INSTALL_DIR ${CMAKE_INSTALL_SYSCONFDIR}/event_dispatcher
#   CACHE PATH "Location of configuration files" )
# set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}
#   CACHE PATH "Location of header files" )

# configure_package_config_file(cmake/event_dispatcher-config.cmake.in
#   ${CMAKE_CURRENT_BINARY_DIR}/event_dispatcherConfig.cmake
#   INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/event_dispatcher
#   PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR)

# write_basic_package_version_file(
#   ${CMAKE_CURRENT_BINARY_DIR}/event_dispatcher-configVersion.cmake
#   VERSION ${PROJECT_VERSION} 
#   COMPATIBILITY SameMajorVersion)

# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/event_dispatcher-config.cmake
#               ${CMAKE_CURRENT_BINARY_DIR}/event_dispatcher-configVersion.cmake
#         DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/event_dispatcher)
