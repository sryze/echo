project(ehlo-server C)
cmake_minimum_required(VERSION 3.0)

add_executable(ehlo-server ehlo-server.c)

if(WIN32)
  target_link_libraries(ehlo-server ws2_32)
endif()
