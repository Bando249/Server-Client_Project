cmake_minimum_required(VERSION 3.15)
project(RadioServer)

# Use C++17 for filesystem support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# Source files
set(SRC_FILES
    src/serverMain.cpp
    src/serverHandles.cpp
    src/serverNetwork.cpp
    src/database.cpp
    libs/sqlite3.c       # compile SQLite directly
    src/server.rc
)

# Create executable
add_executable(server ${SRC_FILES})

# Include directories
target_include_directories(server PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs
)

# Link system libraries
target_link_libraries(server PRIVATE ws2_32)
