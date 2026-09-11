cmake_minimum_required(VERSION 3.20)

project(SKript VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(skript_lexer
    src/lexer/lexer.cpp
    src/lexer/token.cpp
)
target_include_directories(skript_lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(skript_lexer PUBLIC cxx_std_17)

add_library(skript_parser src/parser/parser.cpp)
target_link_libraries(skript_parser PUBLIC skript_lexer)
target_include_directories(skript_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(skript_runtime
    src/runtime/builtins.cpp
    src/runtime/environment.cpp
    src/runtime/interpreter.cpp
    src/runtime/value.cpp
)
target_link_libraries(skript_runtime PUBLIC skript_parser)
target_include_directories(skript_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(skript_compiler
    src/compiler/bytecode.cpp
    src/compiler/compiler.cpp
)
target_link_libraries(skript_compiler PUBLIC skript_runtime)
target_include_directories(skript_compiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(skript_vm src/vm/virtual_machine.cpp)
target_link_libraries(skript_vm PUBLIC skript_compiler)
target_include_directories(skript_vm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(skript src/main.cpp)
target_link_libraries(skript PRIVATE skript_vm)

option(SKRIPT_BUILD_TESTS "Build SKript tests" ON)
if(SKRIPT_BUILD_TESTS)
    include(CTest)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(skript_lexer_tests tests/lexer/lexer_test.cpp)
    target_link_libraries(skript_lexer_tests PRIVATE skript_lexer GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(skript_lexer_tests)

    add_executable(skript_parser_tests tests/parser/parser_test.cpp)
    target_link_libraries(skript_parser_tests PRIVATE skript_parser GTest::gtest_main)
    gtest_discover_tests(skript_parser_tests)

    add_executable(skript_interpreter_tests tests/runtime/interpreter_test.cpp)
    target_link_libraries(skript_interpreter_tests PRIVATE skript_runtime GTest::gtest_main)
    gtest_discover_tests(skript_interpreter_tests)

    add_executable(skript_vm_tests tests/vm/virtual_machine_test.cpp)
    target_link_libraries(skript_vm_tests PRIVATE skript_vm GTest::gtest_main)
    gtest_discover_tests(skript_vm_tests)
endif()
