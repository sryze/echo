project(ehlo C)
cmake_minimum_required(VERSION 3.0)

add_executable(ehlo ehlo.c)

if(WIN32)
  target_link_libraries(ehlo ws2_32)
endif()
