cmake_minimum_required(VERSION 3.24)

project(use_usocket LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES external/uWebSockets/uSockets/src/*.c)
add_library(uSockets ${SOURCES})
target_include_directories(uSockets PUBLIC external/uWebSockets/uSockets/src)
target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)

find_package(ZLIB REQUIRED)
add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE external/uWebSockets/src/)
target_link_libraries(uWebSockets INTERFACE uSockets ${ZLIB_LIBRARIES})
target_compile_options(uWebSockets INTERFACE -Wno-deprecated-declarations)

find_package(uuid_v4)

# Server exec
add_executable(lobby_server main.cpp)
target_link_libraries(lobby_server uuid_v4::uuid_v4)
target_link_libraries(lobby_server uWebSockets)
