cmake_minimum_required(VERSION 3.16)

project(number_merge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(number_merge_lib src/game.cpp)
target_include_directories(number_merge_lib PUBLIC include)

if(NOT WIN32)
    message(FATAL_ERROR "The graphical version currently targets the native Windows API.")
endif()

add_executable(number_merge WIN32 src/main.cpp)
target_link_libraries(number_merge PRIVATE number_merge_lib)
target_link_libraries(number_merge PRIVATE gdi32 user32)

enable_testing()
add_executable(number_merge_tests tests/game_tests.cpp)
target_link_libraries(number_merge_tests PRIVATE number_merge_lib)
add_test(NAME number_merge_tests COMMAND number_merge_tests)
