find_package(OpenCV)

if(NOT ${OpenCV_DIR} MATCHES OpenCV_DIR-NOTFOUND)
  include_directories(..)

  file(GLOB TESTS *.cpp)
  foreach(TEST ${TESTS})
    get_filename_component(TEST_BASENAME ${TEST} NAME_WE)
    add_executable(${TEST_BASENAME} ${TEST})
    add_dependencies(${TEST_BASENAME} OpenCV-2-4-4)
    target_link_libraries(${TEST_BASENAME} likely opencv_core)
    add_test("${TEST_BASENAME}_test" ${TEST_BASENAME})
  endforeach()
endif()

