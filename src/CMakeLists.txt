if(NOT VS)
  if(GCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -fno-rtti")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-x")
endif()

if(GCC)
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s")
endif()

if(APPLE)
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-dead_strip")
endif()

set(LIKELY_SRC likely.cpp ${CMAKE_SOURCE_DIR}/include/likely.h)
set(LIKELY_LIBS ${REQ_LLVM_LIBRARIES})
set(LIKELY_DEPENDS LLVM)

if(${LIKELY_AUX})
  set(LIKELY_SRC ${LIKELY_SRC} likely_aux.cpp ${CMAKE_SOURCE_DIR}/include/likely_aux.h)
  set(LIKELY_LIBS ${LIKELY_LIBS} opencv_highgui)
  set(LIKELY_DEPENDS ${LIKELY_DEPENDS} OpenCV)
  install(FILES ${CMAKE_SOURCE_DIR}/include/likely_aux.h DESTINATION include)
endif()

if(${LIKELY_SCRIPT})
  # Compile in standard.likely as a string
  set(LIKELY_STANDARD_LIBRARY_CMAKE ${CMAKE_BINARY_DIR}/standard_library.cmake)
  set(LIKELY_STANDARD_LIBRARY_CPP ${CMAKE_BINARY_DIR}/likely_standard_library.cpp)
  file(WRITE ${LIKELY_STANDARD_LIBRARY_CMAKE}
       "set(LIKELY_STANDARD_LIBRARY_CPP ${LIKELY_STANDARD_LIBRARY_CPP})
        set(LIKELY_STANDARD_LIBRARY \"${CMAKE_SOURCE_DIR}/share/likely/standard.likely\")
        if (\${LIKELY_STANDARD_LIBRARY} IS_NEWER_THAN \${LIKELY_STANDARD_LIBRARY_CPP})
        file(WRITE \${LIKELY_STANDARD_LIBRARY_CPP} \"// Probably a bad idea to modify this automatically generated source file!\\n\\n\")
        file(APPEND \${LIKELY_STANDARD_LIBRARY_CPP} \"#include \\\"likely_script.h\\\"\\n\\nconst char *likely_standard_library()\\n{\\n\\treturn\\n\\t\\\"\")
        file(READ \${LIKELY_STANDARD_LIBRARY} STANDARD_LIBRARY_SOURCE)
        string(REPLACE \"\\\\\" \"\\\\\\\\\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
        string(REPLACE \"\\\"\" \"\\\\\\\"\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
        string(REPLACE \"\\n\" \"\\\\n\\\"\\n\\t\\\"\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
        file(APPEND \${LIKELY_STANDARD_LIBRARY_CPP} \${STANDARD_LIBRARY_SOURCE})
        file(APPEND \${LIKELY_STANDARD_LIBRARY_CPP} \"\\\";\\n}\\n\")
        endif()")
  include(${LIKELY_STANDARD_LIBRARY_CMAKE}) # Run it once
  add_custom_target(likely_standard_library COMMAND ${CMAKE_COMMAND} -P ${LIKELY_STANDARD_LIBRARY_CMAKE})

  set(LIKELY_SRC ${LIKELY_SRC} likely_script.cpp
                               ${LIKELY_STANDARD_LIBRARY_CPP}
                               ${CMAKE_SOURCE_DIR}/include/likely_script.h
                               ${CMAKE_SOURCE_DIR}/share/likely/standard.likely
                               ${CMAKE_SOURCE_DIR}/share/likely/opencv.shim)
  set(LIKELY_LIBS ${LIKELY_LIBS} liblua)
  set(LIKELY_DEPENDS ${LIKELY_DEPENDS} lua likely_standard_library)
  install(FILES ${CMAKE_SOURCE_DIR}/include/likely_script.h DESTINATION include)
endif()

if(NOT ${BUILD_SHARED_LIBS})
  add_definitions(-DLIKELY_LIBRARY)
endif()

add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)
add_library(likely ${LIKELY_SRC})
add_dependencies(likely ${LIKELY_DEPENDS})
set_target_properties(likely PROPERTIES
                      DEFINE_SYMBOL LIKELY_LIBRARY
                      VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                      SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
target_link_libraries(likely ${LIKELY_LIBS})
install(TARGETS likely
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES ${CMAKE_SOURCE_DIR}/include/likely.h DESTINATION include)
install(DIRECTORY share DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.txt DESTINATION share/likely)

if(${LIKELY_SCRIPT})
  add_custom_command(TARGET likely POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E touch ${LIKELY_STANDARD_LIBRARY_CPP})
endif()

if(${LIKELY_AUX} AND ${LIKELY_SCRIPT})
  # Benchmark
  add_executable(benchmark benchmark.cpp)
  add_dependencies(benchmark OpenCV)
  target_link_libraries(benchmark likely opencv_imgproc)
  install(TARGETS benchmark RUNTIME DESTINATION bin)

  # Dream
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

    add_executable(Dream dream.cpp ${QCUSTOMPLOT_SRC})
    qt5_use_modules(Dream Quick Widgets PrintSupport)
    target_link_libraries(Dream likely)
    install(TARGETS Dream RUNTIME DESTINATION bin)
  endif()
endif()
