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
find_package(nlohmann_json REQUIRED)

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
        nlohmann_json::nlohmann_json
)

# -----------------------------
# Warnings
# -----------------------------
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pattern_finder PRIVATE -Wall -Wextra -Wpedantic)
elseif (MSVC)
    target_compile_options(pattern_finder PRIVATE /W4)
endif()
