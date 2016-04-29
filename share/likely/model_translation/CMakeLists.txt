project(model_translation)
option(LIKELY_MODELTRANSLATION "Build model translation tutorial" ON)

if(LIKELY_MODELTRANSLATION)
  download_demo_file("lfwa_grayscale_mean.lm"  103a2631963155550f1b6347e585a03f)
  download_demo_file("lfwa_grayscale_evecs.lm" e4018236995896304484e966fba35198)

  add_test(NAME lfwa_grayscale_mean_lm
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND likely -q -c "(read-matrix \"data/demo/lfwa_grayscale_mean.lm\")")
  add_test(NAME lfwa_grayscale_evecs_lm
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND likely -q -c "(read-matrix \"data/demo/lfwa_grayscale_evecs.lm\")")

  # Source version
  add_custom_command(OUTPUT eigenfaces_lisp.o
                     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                     COMMAND likely ${CMAKE_CURRENT_SOURCE_DIR}/eigenfaces.lisp ${CMAKE_CURRENT_BINARY_DIR}/eigenfaces_lisp.o
                     DEPENDS likely eigenfaces.lisp)
  set_source_files_properties(eigenfaces_lisp.o PROPERTIES EXTERNAL_OBJECT true GENERATED true)
  add_executable(eigenfaces_lisp eigenfaces_lisp.o)
  set_target_properties(eigenfaces_lisp PROPERTIES LINKER_LANGUAGE C)
  target_link_libraries(eigenfaces_lisp likely_shared)
  if(MSVC)
    target_link_libraries(eigenfaces_lisp MSVCRT)
  endif()
  add_test(NAME eigenfaces_lisp
           COMMAND eigenfaces_lisp ${CMAKE_SOURCE_DIR}/data/lfwa/AJ_Cook/AJ_Cook_0001.jpg)
endif()
