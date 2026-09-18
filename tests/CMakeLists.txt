include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
FetchContent_MakeAvailable(googletest)

add_executable(tp_tests test_smoke.cpp test_thread_pool.cpp)
target_link_libraries(tp_tests PRIVATE tp GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tp_tests)
