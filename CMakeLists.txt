cmake_minimum_required(VERSION 3.16)
project(lilac VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Werror -O3 -save-temps -fverbose-asm")

add_executable(lilac)
target_sources(lilac PRIVATE
    src/main.cpp

    src/lexer.cpp
)

include(CTest)
if(BUILD_TESTING)
    add_executable(lexer_tests tests/lexer_tests.cpp src/lexer.cpp)
    target_include_directories(lexer_tests PRIVATE src)
    add_test(NAME lexer_tests COMMAND lexer_tests)
endif()
