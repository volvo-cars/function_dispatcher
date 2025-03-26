set(EXAMPLES same_signature complex_type safety reference events detaching timer async_call)

foreach(EXAMPLE ${EXAMPLES})
    add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
    target_link_libraries(${EXAMPLE} PUBLIC ${PROJECT_NAME})
endforeach()
