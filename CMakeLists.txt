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
file(GLOB SOURCES "src/*.cpp")

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
  GIT_TAG           master
)

FetchContent_MakeAvailable(readerwriterqueue)

FetchContent_Declare(
  yyjson
  GIT_REPOSITORY https://github.com/ibireme/yyjson.git
  GIT_TAG master  # or specific commit/tag
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(yyjson)

find_package(unofficial-inih CONFIG REQUIRED)

# LuaJIT

# Path to LuaJIT
set(LUAJIT_INCLUDE_DIR "/usr/local/include/luajit-2.1")
set(LUAJIT_LIB_DIR "/usr/local/lib")
set(LUAJIT_LIBRARIES "/usr/local/lib/libluajit-5.1.a")

# Add LuaJIT include directory
include_directories(${LUAJIT_INCLUDE_DIR})

# Angelscript
add_compile_options(-Wno-deprecated-declarations)
add_subdirectory(external/angelscript/sdk/angelscript/projects/cmake)
file(GLOB SCRIPTSTDSTRING_SOURCES external/angelscript/sdk/add_on/scriptstdstring/*.cpp)
file(GLOB SCRIPTARRAY_SOURCES external/angelscript/sdk/add_on/scriptarray/*.cpp)
file(GLOB SCRIPTDICTIONARY_SOURCES external/angelscript/sdk/add_on/scriptdictionary/*.cpp)

add_executable(lobby_server ${SOURCES})
target_link_libraries(lobby_server PRIVATE Boost::uuid Boost::lockfree uWebSockets angelscript readerwriterqueue unofficial::inih::inireader yyjson ${LUAJIT_LIBRARIES})
target_include_directories(lobby_server PRIVATE
  "external/angelscript/sdk/angelscript/include"
  "external/angelscript/sdk/add_on"
  )

target_sources(lobby_server PRIVATE 
    ${SCRIPTSTDSTRING_SOURCES}
    ${SCRIPTARRAY_SOURCES}
    ${SCRIPTDICTIONARY_SOURCES}
)
