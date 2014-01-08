project(hello_world)

# hello_world_jit
if(${LIKELY_IO} AND ${LIKELY_SCRIPT})
  add_executable(hello_world_jit hello_world_jit.c)
  target_link_libraries(hello_world_jit likely)
endif()

# hello_world_static
if(${LIKELY_SCRIPT})
  add_custom_command(OUTPUT hello_world_div2.o
                     COMMAND like "div(arg(0), 2)" hello_world_div2 f32 hello_world_div2.o
                     DEPENDS like
                     VERBATIM)

  add_executable(hello_world_static hello_world_static.c hello_world_div2.o)
  set_source_files_properties(hello_world_div2.o PROPERTIES EXTERNAL_OBJECT true GENERATED true)
  target_link_libraries(hello_world_static likely_runtime)
endif()
