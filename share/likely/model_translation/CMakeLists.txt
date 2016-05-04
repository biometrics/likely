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
  likely_executable(eigenfaces_lisp)
  add_test(NAME eigenfaces_lisp
           COMMAND eigenfaces_lisp ${CMAKE_SOURCE_DIR}/data/lfwa/AJ_Cook/AJ_Cook_0001.jpg)

  # API version
  add_executable(eigenfaces_model_translator eigenfaces_model_translator.c)
  target_link_libraries(eigenfaces_model_translator likely_shared)
  add_custom_command(OUTPUT eigenfaces_api.o
                     COMMAND eigenfaces_model_translator ${CMAKE_CURRENT_SOURCE_DIR}/eigenfaces-api.lisp ${CMAKE_SOURCE_DIR}/data/demo/lfwa_grayscale_mean.lm ${CMAKE_SOURCE_DIR}/data/demo/lfwa_grayscale_evecs.lm ${CMAKE_CURRENT_BINARY_DIR}/eigenfaces_api.o
                     DEPENDS likely eigenfaces-api.lisp)
  likely_executable(eigenfaces_api)
  add_test(NAME eigenfaces_api
           COMMAND eigenfaces_api ${CMAKE_SOURCE_DIR}/data/lfwa/AJ_Cook/AJ_Cook_0001.jpg)
endif()
