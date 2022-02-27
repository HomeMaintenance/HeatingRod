find_package(GTest CONFIG REQUIRED)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_executable(heatingrod_test heatingrod_test.cpp)
target_include_directories(heatingrod_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(heatingrod_test PUBLIC HeatingRod)
target_link_libraries(heatingrod_test PUBLIC GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(heatingrod_test)
