cmake_minimum_required(VERSION 3.2)
project(markov)

include(FindPkgConfig)
pkg_search_module(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
link_libraries(${GLIB2_LIBRARIES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c11")

add_executable(markov ${SOURCE_FILES} markov.c)