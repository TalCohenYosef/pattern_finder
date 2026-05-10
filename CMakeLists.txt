cmake_minimum_required(VERSION 3.16)

project(pattern_finder
    VERSION 1.0
    LANGUAGES CXX
)

# -----------------------------
# C++ standard
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Find Boost
# -----------------------------
find_package(Boost REQUIRED COMPONENTS program_options)

# -----------------------------
# Find nlohmann/json
# -----------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONC REQUIRED json-c)

# -----------------------------
# Collect source files dynamically
# -----------------------------
file(GLOB SRC_FILES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# -----------------------------
# Create executable
# -----------------------------
add_executable(pattern_finder ${SRC_FILES})

# -----------------------------
# Include directories
# -----------------------------
target_include_directories(pattern_finder
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
)

# -----------------------------
# Link libraries
# -----------------------------
target_link_libraries(pattern_finder
    PRIVATE
        Boost::program_options
)
target_link_libraries(pattern_finder
    PRIVATE
        json-c
)

# -----------------------------
# Warnings
# -----------------------------

target_compile_options(pattern_finder PRIVATE -O2 -g)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pattern_finder PRIVATE -Wall -Wextra -Wpedantic)
elseif (MSVC)
    target_compile_options(pattern_finder PRIVATE /W4)
endif()

find_package(GTest REQUIRED)
include(GoogleTest)

# Single-graph tests
add_executable(test_single
    tests/test_single_graph_pattern_finder.cpp
    src/SingleGraphPatternFinder.cpp
    src/SingleGraphHistogram.cpp
    src/PatternScorer.cpp
    src/PatternUtils.cpp
    src/Graph.cpp)
target_include_directories(test_single PRIVATE include tests)
target_link_libraries(test_single GTest::GTest GTest::Main Boost::graph)
gtest_discover_tests(test_single)

# Multi-graph tests
add_executable(test_multi
    tests/test_multi_graph_pattern_finder.cpp
    src/MultiGraphPatternFinder.cpp
    src/GeneralColorHist.cpp
    src/IndevidualColorHist.cpp
    src/PatternUtils.cpp
    src/Tree.cpp
    src/Graph.cpp)
target_include_directories(test_multi PRIVATE include tests)
target_link_libraries(test_multi GTest::GTest GTest::Main Boost::graph)
gtest_discover_tests(test_multi)