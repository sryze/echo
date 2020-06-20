project(ehlo C)
cmake_minimum_required(VERSION 3.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)
  add_definitions(-DWIN32_LEAN_AND_MEAN -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()
if(UNIX)
  add_definitions(-D_GNU_SOURCE)
endif()

add_library(ehlo-shared STATIC ehlo-shared.h ehlo-shared.c)
if(WIN32)
  target_link_libraries(ehlo-shared ws2_32)
endif()
if(UNIX)
  target_link_libraries(ehlo-shared pthread)
endif()

add_executable(ehlo ehlo.c)
target_link_libraries(ehlo ehlo-shared)

add_executable(ehlo-server ehlo-server.c)
target_link_libraries(ehlo-server ehlo-shared)
