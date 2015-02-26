project(hello_world)
option(LIKELY_HELLOWORLD "Build Hello World examples" ON)

if(${LIKELY_HELLOWORLD})
  # hello_world_jit
  add_executable(hello_world_jit hello_world_jit.c)
  target_link_libraries(hello_world_jit likely_shared)
  add_test(NAME hello_world_jit
           COMMAND hello_world_jit ${CMAKE_SOURCE_DIR}/data/misc/lenna.tiff ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello-world-jit.lisp dark_lenna.png)

  # hello_world_static
  add_custom_command(OUTPUT hello_world_static_lisp.o
                     COMMAND likely ${CMAKE_CURRENT_SOURCE_DIR}/hello_world_static.lisp hello_world_static_lisp.o
                     DEPENDS likely hello_world_static.lisp
                     VERBATIM)
  add_definitions(-DLIKELY_LIBRARY)
  add_executable(hello_world_static hello_world_static.c hello_world_static_lisp.o)
  set_source_files_properties(hello_world_static_lisp.o PROPERTIES EXTERNAL_OBJECT true GENERATED true)
  target_link_libraries(hello_world_static likely_runtime)
  add_test(NAME hello_world_static
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND hello_world_static)
endif()
