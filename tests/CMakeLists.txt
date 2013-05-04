option(BUILD_TESTING "Benchmark Likely against OpenCV" OFF)

if(${BUILD_TESTING})
  # Build OpenCV
  ExternalProject_Add(
    OpenCV
    PREFIX OpenCV
    URL http://downloads.sourceforge.net/project/opencvlibrary/opencv-unix/2.4.5/opencv-2.4.5.tar.gz
    URL_MD5 8eac87462c7bec8b89021b723207c623
    CMAKE_ARGS ${EXTERNAL_PROJECT_ARGS} -DCMAKE_BUILD_TYPE=Release)

  # Find OpenCV
  find_package(OpenCV QUIET PATHS ${EXTERNAL_PROJECT_PATH} NO_DEFAULT_PATH)
  if(NOT ${OpenCV_DIR} MATCHES OpenCV_DIR-NOTFOUND)
    enable_testing()
    include_directories(..)

    # Find tests
    file(GLOB TESTS *.cpp)
    file(GLOB TEST_SRC test.cpp)
    list(REMOVE_ITEM TESTS ${TEST_SRC})

    # Build tests
    foreach(TEST ${TESTS})
      get_filename_component(TEST_BASENAME ${TEST} NAME_WE)
      add_executable(${TEST_BASENAME} ${TEST} ${TEST_SRC})
      add_dependencies(${TEST_BASENAME} OpenCV)
      target_link_libraries(${TEST_BASENAME} likely opencv_core)
      add_test("${TEST_BASENAME}_test" ${TEST_BASENAME})
    endforeach()
  endif()
endif()
