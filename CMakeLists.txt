cmake_minimum_required(VERSION 3.2)
project(markov)

add_executable(markov markov.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
target_link_libraries(markov PkgConfig::deps)
