# Engine/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Collect source files
file(GLOB_RECURSE ENGINE_SRC 
    "src/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS 
    "include/*.h"
)

# Create static library
add_library(Engine STATIC ${ENGINE_SRC} ${ENGINE_HEADERS})

# Set include directories
target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set C++ standard
target_compile_features(Engine PUBLIC cxx_std_17)

# Enable threading support
find_package(Threads REQUIRED)
target_link_libraries(Engine PUBLIC Threads::Threads) 