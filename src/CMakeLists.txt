if(NOT VS)
  if(GCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -fno-rtti")
  set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} -Wl,-x")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-x")
endif()

if(GCC)
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s")
endif()

if(APPLE)
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-dead_strip")
endif()

set(LIKELY_SRC likely_core.cpp)
set(LIKELY_LIBS ${REQ_LLVM_LIBRARIES} liblua)
file(GLOB LIKELY_LIBRARY ${CMAKE_SOURCE_DIR}/library/*.like)
set(LIKELY_MISC ${LIKELY_LIBRARY}
                ${CMAKE_SOURCE_DIR}/include/likely.h
                ${CMAKE_SOURCE_DIR}/include/likely/likely_core.h
                ${CMAKE_SOURCE_DIR}/LICENSE.txt
                ${CMAKE_SOURCE_DIR}/README.md
                ${CMAKE_SOURCE_DIR}/index.html)

if(${LIKELY_AUX})
  add_definitions(-DLIKELY_AUX)
  set(LIKELY_SRC ${LIKELY_SRC} likely_aux.cpp)
  set(LIKELY_LIBS ${LIKELY_LIBS} opencv_highgui)
  set(LIKELY_MISC ${LIKELY_MISC} ${CMAKE_SOURCE_DIR}/include/likely/likely_aux.h
                                 opencv.shim)
endif()

if(${LIKELY_SCRIPT})
  add_definitions(-DLIKELY_SCRIPT)
  add_executable(embedfile embedfile.c)
  add_custom_command(OUTPUT likely_standard_library.c
                     COMMAND embedfile likely_standard_library ${CMAKE_SOURCE_DIR}/library/standard.like
                     DEPENDS embedfile ${CMAKE_SOURCE_DIR}/library/standard.like)
  set(LIKELY_SRC ${LIKELY_SRC} likely_script.cpp ${CMAKE_CURRENT_BINARY_DIR}/likely_standard_library.c)
  set(LIKELY_DEPENDS ${LIKELY_DEPENDS} likely_standard_library.c)
  set(LIKELY_MISC ${LIKELY_MISC} ${CMAKE_SOURCE_DIR}/include/likely/likely_script.h)
endif()

# Build common object files
add_definitions(-DLIKELY_LIBRARY -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)
add_library(likely_object OBJECT ${LIKELY_SRC})
add_dependencies(likely_object ${LIKELY_LIBS} ${LIKELY_DEPENDS})

# Build shared library
add_library(likely SHARED $<TARGET_OBJECTS:likely_object> ${LIKELY_MISC})
set_target_properties(likely PROPERTIES VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                                        SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
target_link_libraries(likely ${LIKELY_LIBS})
install(TARGETS likely ARCHIVE DESTINATION lib
                       LIBRARY DESTINATION lib
                       RUNTIME DESTINATION bin)

# Build static library
add_library(likely_static STATIC $<TARGET_OBJECTS:likely_object>)
set_target_properties(likely_static PROPERTIES OUTPUT_NAME likely)
target_link_libraries(likely_static ${LIKELY_LIBS})
install(TARGETS likely_static ARCHIVE DESTINATION lib)

if(${LIKELY_AUX} AND ${LIKELY_SCRIPT})
  # Benchmark
  add_executable(benchmark benchmark.cpp)
  target_link_libraries(benchmark likely opencv_imgproc)
  install(TARGETS benchmark RUNTIME DESTINATION bin)

  add_custom_target(test
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data
                    COMMAND benchmark --test)

  # Dream (likely)
  find_package(Qt5Quick QUIET)
  find_package(Qt5Widgets QUIET)
  find_package(Qt5PrintSupport QUIET)
  if(${Qt5Quick_FOUND} AND ${Qt5Widgets_FOUND} AND ${Qt5PrintSupport_FOUND})
    # Get QCustomPlot source
    # aux_source_directory(qcustomplot/src QCUSTOMPLOT_SRC)
    # aux_source_directory(qcustomplot/src/plottables QCUSTOMPLOT_SRC_PLOTTABLES)
    # aux_source_directory(qcustomplot/src/items QCUSTOMPLOT_SRC_ITEMS)
    # aux_source_directory(qcustomplot/src/layoutelements QCUSTOMPLOT_SRC_LAYOUTELEMENTS)
    # set(QCUSTOMPLOT_SRC ${QCUSTOMPLOT_SRC} ${QCUSTOMPLOT_SRC_PLOTTABLES} ${QCUSTOMPLOT_SRC_ITEMS} ${QCUSTOMPLOT_SRC_LAYOUTELEMENTS})

    set(CMAKE_AUTOMOC ON)
    add_executable(dream dream.cpp ${QCUSTOMPLOT_SRC})
    set_target_properties(dream PROPERTIES OUTPUT_NAME likely)
    qt5_use_modules(dream Quick Widgets PrintSupport)
    target_link_libraries(dream likely_static)
    install(TARGETS dream RUNTIME DESTINATION bin)
  endif()
endif()
