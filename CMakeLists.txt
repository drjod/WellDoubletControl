cmake_minimum_required(VERSION 2.6)
project(WellDoubletControl)
set(WellDoubletControl_VERSION_MAJOR 1)
set(WellDoubletControl_VERSION_MINOR 0)
set(logging 1)
option(GTEST "Use google test" ON)


configure_file(
        "${PROJECT_SOURCE_DIR}/wdc_config.h.in"
        "${PROJECT_BINARY_DIR}/wdc_config.h"
)

include_directories(
        "${PROJECT_BINARY_DIR}"  # for config files
        "${PROJECT_SOURCE_DIR}/src"
)


set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(src)

# add_executable(simulate main.cpp)
# target_link_libraries(simulate fakeSimulator)
# target_link_libraries(simulate wellDoubletControl)
# target_link_libraries(simulate fakeSimulator)

if(GTEST)
		set(CMAKE_CXX_FLAGS "-pthread")
        add_subdirectory(fakeSimulator)
		add_subdirectory("${PROJECT_SOURCE_DIR}/ext/gtest-1.8.0")

        enable_testing()
        #find_package(GTest REQUIRED)

        include_directories(
                "${PROJECT_SOURCE_DIR}/fakeSimulator"
				"${gtest_SOURCE_DIR}"
				"${gtest_SOURCE_DIR}/include"
		)

        add_executable(run_tests gtest/allTests.cpp)
        target_link_libraries(run_tests
								fakeSimulator
								wellDoubletControl
								gtest gtest_main
							)


endif(GTEST)

