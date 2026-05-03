file(GLOB SOURCES "*.c")
file(GLOB PRIVATE_HEADERS "*.h")

add_executable(StreamTests ${SOURCES})
target_link_libraries(StreamTests PRIVATE Stream)
target_link_libraries(StreamTests PRIVATE GKey)
add_test(NAME "tests" COMMAND StreamTests)
