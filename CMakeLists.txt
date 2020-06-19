project(ehlo C)
cmake_minimum_required(VERSION 3.0)

add_library(ehlo-shared STATIC ehlo-shared.c)
if(WIN32)
  target_link_libraries(ehlo-shared ws2_32)
endif()

add_executable(ehlo ehlo.c)
target_link_libraries(ehlo ehlo-shared)

add_executable(ehlo-server ehlo-server.c)
target_link_libraries(ehlo-server ehlo-shared)
