include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(protocol_tests
    test_identifiers.cpp
    test_matrix_types.cpp
    test_jwt.cpp
)

target_link_libraries(protocol_tests PRIVATE
    bsfchat_protocol
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(protocol_tests)
