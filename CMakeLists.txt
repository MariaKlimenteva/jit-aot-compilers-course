cmake_minimum_required(VERSION 3.16)
project(IRBuilderProject VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(IRBuilder STATIC)

set(PROJECT_NAME "IRBuilder")

target_sources(${PROJECT_NAME}
    PRIVATE
        src/Dump.cpp
        # .cpp files
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file(GLOB APP_SOURCES "tests/*.cpp")

add_executable(factorial_builder ${APP_SOURCES})
target_link_libraries(factorial_builder PRIVATE ${PROJECT_NAME})

set(COMMON_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
)

target_compile_options(${PROJECT_NAME} PRIVATE ${COMMON_WARNINGS})
target_compile_options(factorial_builder PRIVATE ${COMMON_WARNINGS})

if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    target_compile_options(factorial_builder PRIVATE -Werror)
endif()

option(BUILD_TESTS "turn on unit tests option" OFF)
if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)
    add_executable(run_tests tests/main_test.cpp)
    target_link_libraries(run_tests PRIVATE ${PROJECT_NAME} GTest::gtest_main)
    add_test(NAME unit_tests COMMAND run_tests)
endif()