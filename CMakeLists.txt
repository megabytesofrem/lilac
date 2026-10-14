cmake_minimum_required(VERSION 3.16)
project(lilac VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Werror -O3 -save-temps -fverbose-asm")

add_executable(lilac)
target_sources(lilac PRIVATE
    src/main.cpp

    src/lexer.cpp
    src/parser.cpp

    # Parser modules
    src/parser/class_parser.cpp
    src/parser/expression_parser.cpp
    src/parser/statement_parser.cpp
    src/parser/selector_call_parser.cpp
    src/parser/type_parser.cpp
)

include(CTest)
if(BUILD_TESTING)
    add_executable(
        statement_parser_tests
        tests/statement_parser_tests.cpp
        src/parser.cpp
        src/parser/expression_parser.cpp
        src/parser/statement_parser.cpp
        src/parser/selector_call_parser.cpp
        src/parser/type_parser.cpp
        src/lexer.cpp
    )
    target_include_directories(statement_parser_tests PRIVATE src)
    add_test(NAME statement_parser_tests COMMAND statement_parser_tests)

    # add_executable(lexer_tests tests/lexer_tests.cpp src/lexer.cpp)
    # target_include_directories(lexer_tests PRIVATE src)
    # add_test(NAME lexer_tests COMMAND lexer_tests)

    # add_executable(ast_visit_tests tests/ast_visit_tests.cpp)
    # target_include_directories(ast_visit_tests PRIVATE src)
    # add_test(NAME ast_visit_tests COMMAND ast_visit_tests)

    # add_executable(
    #     parser_tests
    #     tests/parser_tests.cpp
    #     src/parser.cpp
    #     src/parser/expression_parser.cpp
    #     src/parser/selector_call_parser.cpp
    #     src/parser/type_parser.cpp
    #     src/lexer.cpp
    # )
    # target_include_directories(parser_tests PRIVATE src)
    # add_test(NAME parser_tests COMMAND parser_tests)
endif()
