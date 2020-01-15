cmake_minimum_required(VERSION 3.15)
project(cmake-missed-files-searcher)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} FileSearch.cpp)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

install(TARGETS ${PROJECT_NAME} DESTINATION .)