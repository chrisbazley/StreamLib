file(GLOB SOURCES "*.c")
file(GLOB PRIVATE_HEADERS "*.h")

add_executable(StreamTests ${SOURCES} ${PRIVATE_HEADERS})
target_link_libraries(StreamTests PRIVATE Stream GKey CBUtil)
add_test(NAME "StreamTests" COMMAND StreamTests)
