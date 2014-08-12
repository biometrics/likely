set(LIKELY_SRC backend.cpp frontend.cpp)
file(GLOB LIKELY_LIBRARY ${CMAKE_SOURCE_DIR}/library/*)
set(LIKELY_MISC ${LIKELY_LIBRARY}
                ${CMAKE_SOURCE_DIR}/LICENSE.txt
                ${CMAKE_SOURCE_DIR}/README.md
                ${CMAKE_SOURCE_DIR}/include/likely.h
                ${CMAKE_SOURCE_DIR}/include/likely/backend.h
                ${CMAKE_SOURCE_DIR}/include/likely/export.h
                ${CMAKE_SOURCE_DIR}/include/likely/frontend.h
                ${CMAKE_SOURCE_DIR}/include/likely/runtime.h
                ${CMAKE_SOURCE_DIR}/index.html
                ${CMAKE_SOURCE_DIR}/share/likely/LikelyConfig.cmake
                ${CMAKE_SOURCE_DIR}/share/likely/hello_world/README.ll
                ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello_world_div2.ll
                ${CMAKE_SOURCE_DIR}/src/Doxyfile.in)

# Build standard library
add_executable(embedfile embedfile.c)
add_custom_command(OUTPUT likely_standard_library.c
                   COMMAND embedfile likely_standard_library ${CMAKE_SOURCE_DIR}/library/standard.ll
                   DEPENDS embedfile ${CMAKE_SOURCE_DIR}/library/standard.ll)
set(LIKELY_SRC ${LIKELY_SRC} ${CMAKE_CURRENT_BINARY_DIR}/likely_standard_library.c)
set(LIKELY_DEPENDS ${LIKELY_DEPENDS} likely_standard_library.c)

if(${LIKELY_IO})
  add_definitions(-DLIKELY_IO)
  set(LIKELY_SRC ${LIKELY_SRC} io.cpp opencv.cpp)
  set(LIKELY_MISC ${LIKELY_MISC} ${CMAKE_SOURCE_DIR}/include/likely/io.h
                                 ${CMAKE_SOURCE_DIR}/include/likely/opencv.hpp)
endif()

# CPU Profiling
option(LIKELY_PROFILING "Build Likely for code profiling" OFF)
if(${LIKELY_PROFILING})
  if(NOT VS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  endif()
endif()

# Build runtime-only library
add_definitions(-DLIKELY_LIBRARY)
add_library(likely_runtime STATIC runtime.cpp)

# Build common object files
add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)
add_library(likely_object OBJECT ${LIKELY_SRC})
add_dependencies(likely_object likely_runtime ${LIKELY_LIBS} ${LIKELY_DEPENDS})

# Build shared library
add_library(likely SHARED $<TARGET_OBJECTS:likely_object> ${LIKELY_MISC})
set_target_properties(likely PROPERTIES VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                                        SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
target_link_libraries(likely likely_runtime ${LIKELY_LIBS})
install(TARGETS likely ARCHIVE DESTINATION lib
                       LIBRARY DESTINATION lib
                       RUNTIME DESTINATION bin)

# Build static library
add_library(likely_static STATIC $<TARGET_OBJECTS:likely_object>)
set_target_properties(likely_static PROPERTIES OUTPUT_NAME likely)
target_link_libraries(likely_static likely_runtime ${LIKELY_LIBS})
install(TARGETS likely_static ARCHIVE DESTINATION lib)

# Build static compiler
add_executable(like like.cpp)
set_target_properties(like PROPERTIES OUTPUT_NAME likely)
target_link_libraries(like likely)
install(TARGETS like RUNTIME DESTINATION bin)

if(${LIKELY_IO})
  # Benchmark
  add_executable(benchmark benchmark.cpp)
  target_link_libraries(benchmark likely opencv_imgproc)
  install(TARGETS benchmark RUNTIME DESTINATION bin)
  likely_add_test(benchmark -test)

  # Dream
  find_package(Qt5Quick QUIET)
  find_package(Qt5Widgets QUIET)
  find_package(Qt5PrintSupport QUIET)
  find_package(Qt5OpenGL QUIET)
  if(${Qt5Quick_FOUND} AND ${Qt5Widgets_FOUND} AND ${Qt5PrintSupport_FOUND} AND ${Qt5OpenGL_FOUND})
    # Get QCustomPlot source
    # aux_source_directory(qcustomplot/src QCUSTOMPLOT_SRC)
    # aux_source_directory(qcustomplot/src/plottables QCUSTOMPLOT_SRC_PLOTTABLES)
    # aux_source_directory(qcustomplot/src/items QCUSTOMPLOT_SRC_ITEMS)
    # aux_source_directory(qcustomplot/src/layoutelements QCUSTOMPLOT_SRC_LAYOUTELEMENTS)
    # set(QCUSTOMPLOT_SRC ${QCUSTOMPLOT_SRC} ${QCUSTOMPLOT_SRC_PLOTTABLES} ${QCUSTOMPLOT_SRC_ITEMS} ${QCUSTOMPLOT_SRC_LAYOUTELEMENTS})

    set(CMAKE_AUTOMOC ON)
    add_executable(dream dream.cpp ${QCUSTOMPLOT_SRC})
    qt5_use_modules(dream Quick Widgets PrintSupport OpenGL)
    target_link_libraries(dream likely_static)
    install(TARGETS dream RUNTIME DESTINATION bin)
  endif()
endif()

find_package(Doxygen)
if(${DOXYGEN_FOUND})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in Doxyfile)
  add_custom_target(doc ${DOXYGEN_EXECUTABLE} Doxyfile)
endif()

# Test standard library
function(test_standard NAME EXPR VALUE)
  add_test(NAME ${NAME}
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND like ${EXPR} -assert ${VALUE})
endfunction()

# List in the order they occur in standard.ll
test_standard(e "e" 2.71828)
test_standard(pi "pi" 3.14159)
test_standard(abs "(abs -1.3)" 1.3)
test_standard(sq "(sq -1.3)" 1.69)
test_standard(min "(min -1 1.3)" -1)
test_standard(max "(max 1 -1.3)" 1)

# Test demos
function(test_demo FILE)
  add_test(NAME ${FILE}
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND like library/${FILE}.ll -show -assert "data/demo/${FILE}.jpg")
endfunction()

# List alphabetically
test_demo(gabor_wavelet)
test_demo(mandelbrot_set)

# Test algorithms
option(LIKELY_TEST_ALGORITHMS "Run computationally demanding tests" ON)
if(${LIKELY_TEST_ALGORITHMS})
  test_demo(average_face)
endif()
