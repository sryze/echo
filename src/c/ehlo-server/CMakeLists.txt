add_executable(ehlo-server ehlo-server.c)

if(WIN32)
  target_link_libraries(ehlo-server ws2_32)
endif()
