cmake_minimum_required(VERSION 3.5)

project(asm1802 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

find_package(fmt)

add_executable(asm1802 main.cpp
    sourcecodereader.cpp sourcecodereader.h
    exceptions.h exceptions.cpp
    definemap.h definemap.cpp
    utils.h utils.cpp

    listingfilewriter.h listingfilewriter.cpp
)

install(TARGETS asm1802
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_link_libraries(asm1802 fmt::fmt)
