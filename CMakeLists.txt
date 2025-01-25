cmake_minimum_required(VERSION 3.10)
project(TelegramDrive)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB SRC_FILES src/*.cpp)

add_executable(TelegramDrive ${SRC_FILES})
