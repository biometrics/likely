project(hello_world)
option(LIKELY_HELLOWORLD "Build Hello World examples" ON)

if(${LIKELY_HELLOWORLD})
  # Interpreted
  add_test(NAME hello_world_interpreted
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND likely ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello-world-interpreted.lisp -q)

  # JIT Compiled
  add_executable(hello_world_jit hello_world_jit.c)
  target_link_libraries(hello_world_jit likely_shared)
  add_test(NAME hello_world_jit
           COMMAND hello_world_jit ${CMAKE_SOURCE_DIR}/data/misc/lenna.tiff ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello-world-compiled.lisp dark_lenna.png)

  # Static Compiled
  add_custom_command(OUTPUT hello_world_compiled_lisp.o
                     COMMAND likely ${CMAKE_CURRENT_SOURCE_DIR}/hello-world-compiled.lisp hello_world_compiled_lisp.o
                     DEPENDS likely hello-world-compiled.lisp
                     VERBATIM)
  add_definitions(-DLIKELY_LIBRARY)
  add_executable(hello_world_static hello_world_static.c hello_world_compiled_lisp.o)
  set_source_files_properties(hello_world_compiled_lisp.o PROPERTIES EXTERNAL_OBJECT true GENERATED true)
  target_link_libraries(hello_world_static likely_shared)
  add_test(NAME hello_world_static
           COMMAND hello_world_static ${CMAKE_SOURCE_DIR}/data/misc/lenna.tiff dark_lenna.png)
endif()
