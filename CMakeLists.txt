cmake_minimum_required(VERSION 3.11)
project(StreamLib C)

include(FetchContent)

FetchContent_Declare(GKey GIT_REPOSITORY https://github.com/chrisbazley/GKeyLib.git GIT_TAG main)

FetchContent_MakeAvailable(GKey)

set(CMAKE_C_STANDARD 99)

if(MSVC)
    # MSVC equivalent of -Wall -Wextra is /W4
    add_compile_options(/W4)
else()
    # For GCC and Clang
    add_compile_options(-Wall -Wextra -pedantic)
endif()

set(SOURCES Reader.c ReaderRaw.c ReaderGKey.c ReaderMem.c ReaderNull.c
             ReaderChar.c Reader16.c Reader32.c ReaderSeek.c
             Writer.c WriterRaw.c WriterGKey.c WriterMem.c WriterNull.c
             WriterHeap.c WriterGKC.c WriterChar.c Writer16.c Writer32.c WriterSeek.c)
file(GLOB PUBLIC_HEADERS "*.h")

add_library(Stream STATIC ${SOURCES} ${PUBLIC_HEADERS})
target_link_libraries(Stream PRIVATE GKey)
set_target_properties(Stream PROPERTIES
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    DEBUG_POSTFIX "dbg"
)

target_include_directories(Stream PUBLIC .)

target_compile_definitions(Stream PRIVATE
    $<$<CONFIG:Debug>:DEBUG_OUTPUT>
    $<$<CONFIG:Release>:NDEBUG>
)

target_compile_options(Stream PRIVATE
    $<$<CONFIG:Release>:-O3>
)

install(TARGETS Stream
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

include(CTest)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  add_subdirectory(tests)
endif()
