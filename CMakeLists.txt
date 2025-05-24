cmake_minimum_required(VERSION 3.15)

project(tinyrpc)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAG_DEBUG "-O0 -Wall -g -ggdb")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(BUILD_TESTS CACHE BOOL ON "if to build tests")

set(TINYRPC_ENABLE_PROTOBUF TRUE CACHE BOOL "if to enable protobuf")
set(TINYRPC_DEFAULT_BUFFER_SIZE 1024 CACHE STRING "default buffer size")
set(TINYRPC_VERIFY_FLAG "0xabab" CACHE STRING "verify flag for message")
set(TINYRPC_THREAD_POOL_SIZE 4 CACHE STRING "thread pool size")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS TRUE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_compile_definitions(
        _DEBUG
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
    )
endif()

include(cmake/CPM.cmake)
include(GenerateExportHeader)

find_package(spdlog REQUIRED)
if (TINYRPC_ENABLE_PROTOBUF)
    find_package(protobuf REQUIRED)
endif()

CPMAddPackage(
    NAME asyncio
    GIT_REPOSITORY "https://github.com/kewuaa/asyncio.git"
    GIT_TAG master
    OPTIONS "BUILD_TESTS OFF"
)
CPMAddPackage(
    NAME growable_buffer
    GIT_REPOSITORY "https://github.com/kewuaa/growable_buffer.git"
    GIT_TAG master
    OPTIONS "BUILD_TESTS OFF"
)
CPMAddPackage(
    NAME tiny_thread_pool
    GIT_REPOSITORY "https://github.com/kewuaa/tiny_thread_pool.git"
    GIT_TAG master
    OPTIONS "BUILD_TESTS OFF"
)
CPMAddPackage("gh:msgpack/msgpack-c#cpp-7.0.0")

configure_file(
    include/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/tinyrpc_config.hpp
)

add_library(${PROJECT_NAME}_server)
add_library(${PROJECT_NAME}::server ALIAS ${PROJECT_NAME}_server)
generate_export_header(
    ${PROJECT_NAME}_server
    BASE_NAME ${PROJECT_NAME}
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/tinyrpc_export.hpp
)
target_include_directories(
    ${PROJECT_NAME}_server
    PUBLIC
        include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_sources(
    ${PROJECT_NAME}_server
    PUBLIC
        src/message.cpp
        src/message_parser.cpp
        src/server.cpp
)
target_link_libraries(
    ${PROJECT_NAME}_server
    PUBLIC
        growable_buffer
        asyncio
        spdlog::spdlog
        msgpack-cxx
        tiny_thread_pool
)

add_library(${PROJECT_NAME}_client)
add_library(${PROJECT_NAME}::client ALIAS ${PROJECT_NAME}_client)
target_include_directories(
    ${PROJECT_NAME}_client
    PUBLIC
        include
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_sources(
    ${PROJECT_NAME}_client
    PUBLIC
        src/message.cpp
        src/message_parser.cpp
        src/client.cpp
)
target_link_libraries(
    ${PROJECT_NAME}_client
    PUBLIC
        growable_buffer
        asyncio
        spdlog::spdlog
        msgpack-cxx
)

if (TINYRPC_ENABLE_PROTOBUF)
    add_compile_definitions(TINYRPC_ENABLE_PROTOBUF)
    target_link_libraries(
        ${PROJECT_NAME}_server
        PUBLIC
            protobuf::libprotoc
            protobuf::libprotobuf
    )
    target_link_libraries(
        ${PROJECT_NAME}_client
        PUBLIC
            protobuf::libprotoc
            protobuf::libprotobuf
    )
endif()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()
