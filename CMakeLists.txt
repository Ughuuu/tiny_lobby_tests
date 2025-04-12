cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# uWebSocket
project(use_usocket LANGUAGES C CXX)
file(GLOB_RECURSE SOURCES external/uWebSockets/uSockets/src/*.c)
add_library(uSockets ${SOURCES})
target_include_directories(uSockets PUBLIC external/uWebSockets/uSockets/src)
target_compile_definitions(uSockets PRIVATE LIBUS_NO_SSL)
# zlib
find_package(ZLIB REQUIRED)
add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE external/uWebSockets/src/)
target_link_libraries(uWebSockets INTERFACE uSockets ZLIB::ZLIB)

# Boost
find_package(boost_uuid REQUIRED CONFIG)
find_package(boost_container REQUIRED CONFIG)
file(GLOB SOURCES "src/*.cpp")

# readerwriterqueue
include(FetchContent)
FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
  GIT_TAG           master
)
FetchContent_MakeAvailable(readerwriterqueue)

# yyjson
FetchContent_Declare(
  yyjson
  GIT_REPOSITORY https://github.com/ibireme/yyjson.git
  GIT_TAG master  # or specific commit/tag
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(yyjson)

# Dependencies from vcpkg
find_package(unofficial-inih CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
set(CPPHTTPLIB_OPENSSL_SUPPORT ON)
find_package(httplib CONFIG REQUIRED)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
find_package(unofficial-luau CONFIG REQUIRED)

# Rnp
set(RNP_INSTALL_DIR ${CMAKE_SOURCE_DIR}/external/rnp/build-static)
include_directories(${RNP_INSTALL_DIR}/include)
link_directories(${RNP_INSTALL_DIR}/lib)
add_library(rnp STATIC IMPORTED)
target_include_directories(rnp INTERFACE ${RNP_INSTALL_DIR}/include)
add_library(sexpp STATIC IMPORTED)
find_package(botan CONFIG REQUIRED)
find_package(json-c CONFIG REQUIRED)
find_package(BZip2 REQUIRED)
target_link_libraries(rnp INTERFACE ${JSONC_LIBRARIES} ${BZIP2_LIBRARIES})
target_include_directories(rnp INTERFACE ${JSONC_INCLUDE_DIRS} ${BZIP2_INCLUDE_DIRS})

# RNP
if(MSVC)
  set_target_properties(rnp PROPERTIES
    IMPORTED_LOCATION ${RNP_INSTALL_DIR}/lib/rnp.lib
    INTERFACE_INCLUDE_DIRECTORIES ${RNP_INSTALL_DIR}/include
  )
  set_target_properties(sexpp PROPERTIES
      IMPORTED_LOCATION ${RNP_INSTALL_DIR}/lib/sexpp.lib
  )
else()
  set_target_properties(rnp PROPERTIES
    IMPORTED_LOCATION ${RNP_INSTALL_DIR}/lib/librnp.a
    INTERFACE_INCLUDE_DIRECTORIES ${RNP_INSTALL_DIR}/include
  )
  set_target_properties(sexpp PROPERTIES
      IMPORTED_LOCATION ${RNP_INSTALL_DIR}/lib/libsexpp.a
  )
endif()
# Link libsexpp to rnp
target_link_libraries(rnp INTERFACE sexpp)

# Public Key
file(READ "${CMAKE_SOURCE_DIR}/public.key" PUBLIC_KEY_CONTENTS)
set(PUBLIC_KEY "${PUBLIC_KEY_CONTENTS}")
configure_file(
    ${CMAKE_SOURCE_DIR}/src/public_key.h.in
    ${CMAKE_SOURCE_DIR}/src/public_key.h
    @ONLY
)

add_executable(lobby_server ${SOURCES})

# AngelScript
add_compile_options(-Wno-deprecated-declarations)
add_subdirectory(external/angelscript/sdk/angelscript/projects/cmake)
file(GLOB SCRIPTSTDSTRING_SOURCES external/angelscript/sdk/add_on/scriptstdstring/*.cpp)
file(GLOB SCRIPTGRID_SOURCES external/angelscript/sdk/add_on/scriptgrid/*.cpp)
file(GLOB SCRIPTARRAY_SOURCES external/angelscript/sdk/add_on/scriptarray/*.cpp)
file(GLOB SCRIPTDICTIONARY_SOURCES external/angelscript/sdk/add_on/scriptdictionary/*.cpp)

target_include_directories(lobby_server PRIVATE
  "external/angelscript/sdk/angelscript/include"
  "external/angelscript/sdk/add_on"
  )

target_link_libraries(lobby_server PRIVATE
  Boost::uuid
  Boost::container
  uWebSockets
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
  httplib::httplib
  rnp
  Botan::Botan-static
  json-c::json-c)

if(NOT WIN32)
  find_library(ATOMIC_LIB atomic)
  if (ATOMIC_LIB)
      target_link_libraries(lobby_server PRIVATE ${ATOMIC_LIB})
  endif()
endif()

target_sources(lobby_server PRIVATE 
    ${SCRIPTGRID_SOURCES}
    ${SCRIPTSTDSTRING_SOURCES}
    ${SCRIPTARRAY_SOURCES}
    ${SCRIPTDICTIONARY_SOURCES}
)
