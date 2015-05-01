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
                     COMMAND likely -ro ${CMAKE_CURRENT_SOURCE_DIR}/hello-world-compiled.lisp hello_world_compiled_lisp.o
                     DEPENDS likely hello-world-compiled.lisp)
  set_source_files_properties(hello_world_compiled_lisp.o PROPERTIES EXTERNAL_OBJECT true GENERATED true)
  add_definitions(-DLIKELY_LIBRARY)
  add_executable(hello_world_static hello_world_static.c hello_world_compiled_lisp.o)
  target_link_libraries(hello_world_static likely_shared)
  add_test(NAME hello_world_static
           COMMAND hello_world_static ${CMAKE_SOURCE_DIR}/data/misc/lenna.tiff dark_lenna.png)

  # Trained
  add_executable(hello_world_trained hello_world_trained.c)
  target_link_libraries(hello_world_trained likely_shared)
  add_test(NAME hello_world_trained
           COMMAND hello_world_trained ${CMAKE_CURRENT_SOURCE_DIR}/hello-world-trained.lisp 2 ${CMAKE_BINARY_DIR}/hello_world_precompiled_lisp.bc)

  # Pre-compiled
  add_executable(hello_world_precompiled hello_world_precompiled.c)
  target_link_libraries(hello_world_precompiled likely_shared)
  add_test(NAME hello_world_precompiled
           COMMAND hello_world_precompiled ${CMAKE_SOURCE_DIR}/data/misc/lenna.tiff ${CMAKE_BINARY_DIR}/hello_world_precompiled_lisp.bc hello_world dark_lenna.png)

  # Main
  add_custom_command(OUTPUT hello_world_main_lisp.o
                     COMMAND likely ${CMAKE_CURRENT_SOURCE_DIR}/hello-world-main.lisp hello_world_main_lisp.o
                     DEPENDS likely hello-world-main.lisp)
  set_source_files_properties(hello_world_main_lisp.o PROPERTIES EXTERNAL_OBJECT true GENERATED true)
  add_executable(hello_world_main hello_world_main_lisp.o)
  set_target_properties(hello_world_main PROPERTIES LINKER_LANGUAGE C)
  target_link_libraries(hello_world_main likely_shared)
  if(VS)
    target_link_libraries(hello_world_main MSVCRT)
  endif()
  add_test(NAME hello_world_main
           COMMAND hello_world_main ${CMAKE_SOURCE_DIR}/data/misc/lenna.tiff dark_lenna.png)
endif()
