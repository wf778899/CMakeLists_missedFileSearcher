cmake_minimum_required(VERSION 3.15)
project(MissingFiles_Searcher)

set(CMAKE_CXX_STANDARD 17)

<<<<<<< HEAD
add_executable(FileSearch FileSearch.cpp)
=======
add_executable(FileSearch FileSearch.cpp)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
install(TARGETS FileSearch DESTINATION .)
>>>>>>> First release.
