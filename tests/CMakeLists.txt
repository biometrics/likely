include_directories(..)

# Find tests
file(GLOB TESTS *.cpp)
file(GLOB TEST_CORE test.cpp)
list(REMOVE_ITEM TESTS ${TEST_CORE})

# Build tests
foreach(TEST ${TESTS})
  get_filename_component(TEST_BASENAME ${TEST} NAME_WE)
  set(TEST_BASENAME likely_${TEST_BASENAME})
  add_executable(${TEST_BASENAME} ${TEST} ${TEST_CORE})
  add_dependencies(${TEST_BASENAME} OpenCV)
  target_link_libraries(${TEST_BASENAME} likely opencv_core opencv_imgproc)
  add_test("${TEST_BASENAME}_test" ${TEST_BASENAME} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endforeach()
