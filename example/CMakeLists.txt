# Define the list of executable names
set(EXAMPLES hello_world same_signature)

# Loop through each item in the list
foreach(EXAMPLE ${EXAMPLES})
    add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
    target_link_libraries(${EXAMPLE} PRIVATE ${PROJECT_NAME})
endforeach()