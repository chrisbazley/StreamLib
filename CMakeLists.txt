cmake_minimum_required(VERSION 3.11)
project(StreamLib C)

include(FetchContent)

FetchContent_Declare(gkey GIT_REPOSITORY git@github.com:chrisbazley/GKeyLib.git GIT_TAG main)

FetchContent_MakeAvailable(gkey)


set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
add_compile_options(-Wall -Wextra -pedantic)

set(SOURCES Reader.c ReaderRaw.c ReaderGKey.c ReaderMem.c ReaderNull.c
             ReaderChar.c Reader16.c Reader32.c ReaderSeek.c
             Writer.c WriterRaw.c WriterGKey.c WriterMem.c WriterNull.c
             WriterHeap.c WriterGKC.c WriterChar.c Writer16.c Writer32.c WriterSeek.c)
file(GLOB PUBLIC_HEADERS "*.h")

add_library(Stream STATIC ${SOURCES} ${PUBLIC_HEADERS})
target_link_libraries(Stream PRIVATE gkey)
set_target_properties(Stream PROPERTIES
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    DEBUG_POSTFIX "dbg"
)

target_include_directories(Stream PUBLIC .)
target_compile_definitions(Stream PRIVATE $<$<CONFIG:Debug>:DEBUG_OUTPUT>)
target_link_libraries(Stream PRIVATE gkey)
install(TARGETS Stream
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
