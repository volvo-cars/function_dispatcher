cmake_minimum_required(VERSION 3.24)

project(event_dispatcher VERSION 0.1.0) 
set(CMAKE_CXX_STANDARD 14)

option(${PROJECT_NAME}_example "Build examples" ON)
option(${PROJECT_NAME}_benchmark "Build benchmark" ON)
option(${PROJECT_NAME}_install "Install library" ON)

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE FILE_SET public_headers TYPE HEADERS 
  BASE_DIRS include
  FILES include/${PROJECT_NAME}.hpp)

add_compile_options(-fsanitize=undefined)
add_link_options(-fsanitize=undefined)

include(GNUInstallDirs)
target_include_directories(
	${PROJECT_NAME} INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(${PROJECT_NAME}_example)
	add_subdirectory(example)
endif()

if(${PROJECT_NAME}_benchmark)
	add_subdirectory(benchmark)
endif()

if(${PROJECT_NAME}_install)
	include(cmake/install.cmake)
endif()