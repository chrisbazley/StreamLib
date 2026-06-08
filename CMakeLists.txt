cmake_minimum_required(VERSION 3.11)
project(StreamLib C)

include(FetchContent)

FetchContent_Declare(GKey GIT_REPOSITORY https://github.com/chrisbazley/GKeyLib.git GIT_TAG main)

FetchContent_MakeAvailable(GKey)

set(CMAKE_C_STANDARD 99)
set(CMAKE_XCODE_ATTRIBUTE_RUN_CLANG_STATIC_ANALYZER "YES")

if(MSVC)
    # MSVC equivalent of -Wall -Wextra is /W4
    add_compile_options(/W4 /wd4200 /wd4996 /wd4130)
else()
    # For GCC and Clang
    add_compile_options(-Wall -Wextra -pedantic)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang")
    find_program(CLANG_TIDY_EXE
        NAMES clang-tidy
        HINTS "/opt/homebrew/opt/llvm/bin"
    )

    if(CLANG_TIDY_EXE)
        # 1. Base Clang-Tidy call configuration
        set(CLANG_TIDY_ARGS "${CLANG_TIDY_EXE};-checks=-*,clang-analyzer-*,-clang-analyzer-security.insecureAPI.*")

        # 2. On Apple machines, append the current platform SDK directory path
        if(APPLE)
            execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MAC_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
            list(APPEND CLANG_TIDY_ARGS "--extra-arg=-isysroot" "--extra-arg=${MAC_SDK_PATH}")
        endif()

        # 3. Apply the modified list to CMake
        set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_ARGS}")
        message(STATUS "Clang-Tidy Static Analyzer enabled automatically.")
    else()
        message(STATUS "clang-tidy binary not found. Skipping analysis pass.")
    endif()
endif()

set(SOURCES Reader.c ReaderRaw.c ReaderGKey.c ReaderMem.c ReaderNull.c
             ReaderChar.c Reader16.c Reader32.c ReaderSeek.c
             Writer.c WriterRaw.c WriterGKey.c WriterMem.c WriterNull.c
             WriterHeap.c WriterGKC.c WriterChar.c Writer16.c Writer32.c WriterSeek.c)
file(GLOB PUBLIC_HEADERS "*.h")
file(GLOB PRIVATE_HEADERS "Internal/*.h")

add_library(Stream STATIC ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
target_link_libraries(Stream PRIVATE GKey)
set_target_properties(Stream PROPERTIES
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    DEBUG_POSTFIX "dbg"
)

target_include_directories(Stream PUBLIC .)

target_compile_definitions(Stream PRIVATE
    $<$<CONFIG:Debug>:DEBUG_OUTPUT>
)

install(TARGETS Stream
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

include(CTest)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  add_subdirectory(tests)
endif()
