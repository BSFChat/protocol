cmake_minimum_required(VERSION 3.20)
project(bsfchat-protocol VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GAMECHAT_PROTOCOL_BUILD_TESTS "Build protocol library tests" ON)

include(cmake/Dependencies.cmake)

add_library(bsfchat_protocol STATIC
    src/MatrixTypes.cpp
    src/Identifiers.cpp
    src/JwtUtils.cpp
    src/ErrorCodes.cpp
)

target_include_directories(bsfchat_protocol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(bsfchat_protocol PUBLIC
    nlohmann_json::nlohmann_json
)

target_link_libraries(bsfchat_protocol PRIVATE
    jwt-cpp::jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(GAMECHAT_PROTOCOL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
