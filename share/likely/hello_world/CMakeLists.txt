project(hello_world)
option(LIKELY_HELLOWORLD "Build Likely Hello World examples" ON)

if(${LIKELY_HELLOWORLD})
  # hello_world_jit
  if(${LIKELY_IO})
    add_executable(hello_world_jit hello_world_jit.c)
    target_link_libraries(hello_world_jit likely)
    likely_add_test(hello_world_jit)
  endif()

  # hello_world_static
  add_custom_command(OUTPUT hello_world_div2.o
                     COMMAND like ${CMAKE_CURRENT_SOURCE_DIR}/hello_world_div2.ll hello_world_div2.o
                     DEPENDS like
                     VERBATIM)
  add_definitions(-DLIKELY_LIBRARY)
  add_executable(hello_world_static hello_world_static.c hello_world_div2.o)
  set_source_files_properties(hello_world_div2.o PROPERTIES EXTERNAL_OBJECT true GENERATED true)
  target_link_libraries(hello_world_static likely_runtime)
  likely_add_test(hello_world_static)
endif()
