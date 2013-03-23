find_package(OpenCV)

if(NOT ${OpenCV_DIR} MATCHES OpenCV_DIR-NOTFOUND)
  include_directories(..)

  file(GLOB TESTS *.cpp)
  file(GLOB TEST_SRC test.cpp)
  list(REMOVE_ITEM TESTS ${TEST_SRC})

  foreach(TEST ${TESTS})
    get_filename_component(TEST_BASENAME ${TEST} NAME_WE)
    add_executable(${TEST_BASENAME} ${TEST} ${TEST_SRC})
    add_dependencies(${TEST_BASENAME} OpenCV-2-4-4)
    target_link_libraries(${TEST_BASENAME} likely opencv_core)
    add_test("${TEST_BASENAME}_test" ${TEST_BASENAME})
  endforeach()
endif()

