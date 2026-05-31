cmake_minimum_required(VERSION 3.20)
project(gamev2)

set(CMAKE_CXX_STANDARD 17)

# 1. Find the raylib package
# Note: This assumes you installed raylib via Homebrew (brew install raylib)
find_package(raylib REQUIRED)

# 2. Define your executable
add_executable(gamev2 main.cpp videoPlayer.h)

# 3. LINK the library (This is the part you are missing!)
target_link_libraries(gamev2 PRIVATE raylib)