cmake_minimum_required(VERSION 3.16)
project(gha_demo_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(calculator src/calculator.cpp)
target_include_directories(calculator PUBLIC include)

add_executable(gha_demo_app src/main.cpp)
target_link_libraries(gha_demo_app PRIVATE calculator)

option(BUILD_TESTING_DEMO "Build the test suite" ON)
if(BUILD_TESTING_DEMO)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    # Windows: keep the same runtime library as the rest of the project
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(calculator_tests tests/test_calculator.cpp)
    target_link_libraries(calculator_tests PRIVATE calculator GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(calculator_tests)
endif()