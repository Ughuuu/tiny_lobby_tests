cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_definitions(_WIN32_WINNT=0x0601)
endif()
project(use_usocket LANGUAGES C CXX)
file(GLOB SOURCES "src/*.cpp")

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

# AngelScript
add_subdirectory(external/angelscript/sdk/angelscript/projects/cmake)
file(GLOB SCRIPTSTDSTRING_SOURCES external/angelscript/sdk/add_on/scriptstdstring/*.cpp)
file(GLOB SCRIPTGRID_SOURCES external/angelscript/sdk/add_on/scriptgrid/*.cpp)
file(GLOB SCRIPTANY_SOURCES external/angelscript/sdk/add_on/scriptany/*.cpp)
file(GLOB SCRIPTARRAY_SOURCES external/angelscript/sdk/add_on/scriptarray/*.cpp)
file(GLOB SCRIPTDICTIONARY_SOURCES external/angelscript/sdk/add_on/scriptdictionary/*.cpp)
file(GLOB SCRIPTMATH_SOURCES external/angelscript/sdk/add_on/scriptmath/*.cpp)
file(GLOB SCRIPTBUILDER_SOURCES external/angelscript/sdk/add_on/scriptbuilder/*.cpp)
file(GLOB WEAKREF_SOURCES external/angelscript/sdk/add_on/weakref/*.cpp)
file(GLOB SCRIPTHANDLE_SOURCES external/angelscript/sdk/add_on/scripthandle/*.cpp)
file(GLOB DATETIME_SOURCES external/angelscript/sdk/add_on/datetime/*.cpp)
file(GLOB SCRIPTHELPER_SOURCES external/angelscript/sdk/add_on/scripthelper/*.cpp)

target_include_directories(tiny_lobby PRIVATE "external/angelscript/sdk/angelscript/include"
  "external/angelscript/sdk/add_on")

target_link_libraries(tiny_lobby PRIVATE
  Boost::uuid
  Boost::container
  Boost::beast
  ZLIB::ZLIB
  $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_shared>,libdeflate::libdeflate_shared,libdeflate::libdeflate_static>
  unofficial::uwebsockets::uwebsockets
  angelscript
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

target_sources(tiny_lobby PRIVATE
    ${SCRIPTHELPER_SOURCES}
    ${DATETIME_SOURCES}
    ${SCRIPTHANDLE_SOURCES}
    ${WEAKREF_SOURCES}
    ${SCRIPTBUILDER_SOURCES}
    ${SCRIPTMATH_SOURCES}
    ${SCRIPTANY_SOURCES}
    ${SCRIPTGRID_SOURCES}
    ${SCRIPTSTDSTRING_SOURCES}
    ${SCRIPTARRAY_SOURCES}
    ${SCRIPTDICTIONARY_SOURCES}
)
