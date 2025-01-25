cmake_minimum_required(VERSION 3.10)
project(TelegramDrive)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB SRC_FILES src/*.cpp)

add_executable(TelegramDrive ${SRC_FILES})

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(TelegramDrive PRIVATE -Wall -Wextra -Wpedantic -Wreturn-type)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(TelegramDrive PRIVATE /W4 /WX)
endif()
