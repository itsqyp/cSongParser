cmake_minimum_required(VERSION 3.20)

project(
    CSongParser
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(
    CSongParser
    src/main.c
    src/parser.c
    src/format.c
    src/metadata.c
    src/encoder.c
    src/decoder.c
    src/checksum.c
    src/chunker.c
    src/reassembler.c
)

target_link_libraries(CSongParser PRIVATE bcrypt)

target_include_directories(
    CSongParser
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(
        CSongParser
        PRIVATE
            /W4
    )
else()
    target_compile_options(
        CSongParser
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()