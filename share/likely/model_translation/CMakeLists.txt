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
endif()
