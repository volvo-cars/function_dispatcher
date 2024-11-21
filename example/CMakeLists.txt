set(EXAMPLES hello_world same_signature complex_type safety reference events detaching)

foreach(EXAMPLE ${EXAMPLES})
    add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
    target_include_directories(${EXAMPLE} PUBLIC ${CMAKE_SOURCE_DIR}/include)
endforeach()