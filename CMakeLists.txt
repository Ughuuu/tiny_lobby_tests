cmake_minimum_required(VERSION 3.24)

project(use_usocket LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

file(GLOB_RECURSE SOURCES external/uWebSockets/uSockets/src/*.c)
add_library(uSockets ${SOURCES})
target_include_directories(uSockets PUBLIC external/uWebSockets/uSockets/src)
target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)

find_package(ZLIB REQUIRED)
add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE external/uWebSockets/src/)
target_link_libraries(uWebSockets INTERFACE uSockets ${ZLIB_LIBRARIES})
target_compile_options(uWebSockets INTERFACE -Wno-deprecated-declarations)

find_package(boost_uuid REQUIRED CONFIG)
find_package(boost_lockfree REQUIRED COMPONENTS lockfree)
file(GLOB SOURCES "src/*.cpp")

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
  GIT_TAG           master
)

FetchContent_MakeAvailable(readerwriterqueue)

find_package(Simdjson CONFIG REQUIRED)

add_executable(lobby_server ${SOURCES})
target_link_libraries(lobby_server PRIVATE Boost::uuid Boost::lockfree uWebSockets simdjson::simdjson)
target_link_libraries(lobby_server PUBLIC readerwriterqueue)
