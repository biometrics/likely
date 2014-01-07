project(hello_world)

# hello_world_jit
if(${LIKELY_IO} AND ${LIKELY_SCRIPT})
  add_executable(hello_world_jit hello_world_jit.c)
  target_link_libraries(hello_world_jit likely)
endif()

# hello_world_static
if(${LIKELY_SCRIPT})
  add_executable(hello_world_static hello_world_static.c)
endif()
