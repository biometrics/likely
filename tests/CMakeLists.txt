option(BUILD_TESTING "Benchmark Likely against OpenCV" OFF)
if(${BUILD_TESTING})
  # Find OpenCV
  ExternalProject_Add(
    OpenCV-2-4-3
    PREFIX OpenCV
    URL http://downloads.sourceforge.net/project/opencvlibrary/opencv-unix/2.4.3/OpenCV-2.4.3.tar.bz2
    URL_MD5 c0a5af4ff9d0d540684c0bf00ef35dbe)

  find_package(OpenCV REQUIRED)
  include_directories(..)

  file(GLOB TESTS *.cpp)
  foreach(TEST ${TESTS})
    get_filename_component(TEST_BASENAME ${TEST} NAME_WE)
    add_executable(${TEST_BASENAME} ${TEST})
    target_link_libraries(${TEST_BASENAME} likely ${OpenCV_LIBS})
    add_test("${TEST_BASENAME}_test" ${TEST_BASENAME})
  endforeach()
endif()
