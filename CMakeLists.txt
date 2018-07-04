cmake_minimum_required(VERSION 2.6)
project(WellDoubletControl)
set(WellDoubletControl_VERSION_MAJOR 1)
set(WellDoubletControl_VERSION_MINOR 0)
set(logging 1)

enable_testing()
find_package(GTest REQUIRED)

configure_file(
	"${PROJECT_SOURCE_DIR}/wdc_config.h.in"
	"${PROJECT_BINARY_DIR}/wdc_config.h"
)

include_directories(
	"${PROJECT_BINARY_DIR}"  # for config files
	"${PROJECT_SOURCE_DIR}/fakeSimulator"
	"${PROJECT_SOURCE_DIR}/src"
	"${GTEST_INCLUDE_DIRS}"
)


set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -pthread")

add_subdirectory(fakeSimulator)
add_subdirectory(src)


add_executable(simulate main.cpp)
target_link_libraries(simulate fakeSimulator)
target_link_libraries(simulate wellDoubletControl)


add_executable(allTests gtest/allTests.cpp)
target_link_libraries(allTests fakeSimulator)
target_link_libraries(allTests wellDoubletControl)
target_link_libraries(allTests ${GTEST_BOTH_LIBRARIES})

add_test(ALL allTests)
