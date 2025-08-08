cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_definitions(_WIN32_WINNT=0x0601)
endif()
project(use_usocket LANGUAGES C CXX)
file(GLOB SOURCES "src/*.cpp" "src/websocket/*.cpp" "src/lua/*.cpp" "src/game/*.cpp" "src/authentication/*.cpp" "src/database/*.cpp" "src/common/*.cpp")

if(MSVC)
    # Use Multi-threaded (/MT) for all configurations
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(libdeflate CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(boost_uuid REQUIRED CONFIG)
find_package(boost_container REQUIRED CONFIG)
find_package(boost_beast REQUIRED CONFIG)
find_package(unofficial-inih CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(efsw CONFIG REQUIRED)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
find_package(unofficial-luau CONFIG REQUIRED)
include(FetchContent)
FetchContent_Declare(readerwriterqueue GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue
  GIT_TAG master)
FetchContent_MakeAvailable(readerwriterqueue)
FetchContent_Declare(yyjson GIT_REPOSITORY https://github.com/ibireme/yyjson.git
  GIT_TAG master GIT_SHALLOW TRUE )
FetchContent_MakeAvailable(yyjson)

add_executable(tiny_lobby ${SOURCES})

target_link_libraries(tiny_lobby PRIVATE
  Boost::uuid
  Boost::container
  Boost::beast
  ZLIB::ZLIB
  $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
  unofficial::uwebsockets::uwebsockets
  readerwriterqueue
  unofficial::inih::inireader
  yyjson
  unofficial::luau::Luau.Compiler
  unofficial::luau::Luau.Config
  unofficial::luau::Luau.VM
  unofficial::luau::Luau.Ast
  unofficial::luau::Luau.EqSat
  unofficial::luau::Luau.Common
  libpqxx::pqxx
  OpenSSL::SSL
  OpenSSL::Crypto
  efsw::efsw)
