project(likely)
cmake_minimum_required(VERSION 2.8.9)

# Configure
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CPACK_PACKAGE_NAME "Likely")
set(CPACK_PACKAGE_DESCRIPTION "Literate Kernel Library")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CMAKE_AUTOMOC ON)

# C++11 Compiler flags
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")

macro(get_subdirectories result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      set(dirlist ${dirlist} ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

# Build LLVM
set(LLVM_TARGETS_TO_BUILD "host" CACHE STRING "LLVM targets")
set(LLVM_INCLUDE_TOOLS OFF CACHE BOOL "Build LLVM tools")
set(LLVM_INCLUDE_EXAMPLES OFF CACHE BOOL "Build LLVM examples")
set(LLVM_INCLUDE_TESTS OFF CACHE BOOL "Build LLVM tests")
set(LLVM_INCLUDE_DOCS OFF CACHE BOOL "Build LLVM docs")
add_subdirectory(llvm)
set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/llvm/share/llvm/cmake/")
include(LLVMConfig)
include_directories(${CMAKE_SOURCE_DIR}/llvm/include)
include_directories(${CMAKE_BINARY_DIR}/llvm/include)
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libraries(REQ_LLVM_LIBRARIES mcjit native vectorize)

# Build Lua
add_subdirectory(lua)
include_directories(lua/src ${CMAKE_BINARY_DIR}/lua)

# Build OpenCV
set(WITH_CUDA OFF)
get_subdirectories(DISABLED_OPENCV_MODULES "${CMAKE_SOURCE_DIR}/opencv/modules")
set(REQUIRED_OPENCV_MODULES core imgproc highgui)
list(REMOVE_ITEM DISABLED_OPENCV_MODULES ${REQUIRED_OPENCV_MODULES}) # Disable all but these
foreach(OPENCV_MODULE ${DISABLED_OPENCV_MODULES})
  set(BUILD_opencv_${OPENCV_MODULE}_INIT OFF)
endforeach()
add_subdirectory(opencv)
foreach(OPENCV_MODULE ${REQUIRED_OPENCV_MODULES})
  include_directories(opencv/modules/${OPENCV_MODULE}/include)
endforeach()

# Compile in standard.likely as a string
set(STANDARD_LIBRARY_CMAKE ${CMAKE_BINARY_DIR}/standard_library.cmake)
set(STANDARD_LIBRARY_CPP ${CMAKE_BINARY_DIR}/likely_standard_library.cpp)
file(WRITE ${STANDARD_LIBRARY_CMAKE}
"set(STANDARD_LIBRARY_CPP ${STANDARD_LIBRARY_CPP})
set(STANDARD_LIBRARY \"${CMAKE_SOURCE_DIR}/standard.likely\")
if (\${STANDARD_LIBRARY} IS_NEWER_THAN \${STANDARD_LIBRARY_CPP})
  file(WRITE \${STANDARD_LIBRARY_CPP} \"// Probably a bad idea to modify this automatically generated source file!\\n\\n\")
  file(APPEND \${STANDARD_LIBRARY_CPP} \"#include \\\"likely.h\\\"\\n\\nconst char *likely_standard_library()\\n{\\n\\treturn\\n\\t\\\"\")
  file(READ \${STANDARD_LIBRARY} STANDARD_LIBRARY_SOURCE)
  string(REPLACE \"\\\\\" \"\\\\\\\\\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
  string(REPLACE \"\\\"\" \"\\\\\\\"\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
  string(REPLACE \"\\n\" \"\\\\n\\\"\\n\\t\\\"\" STANDARD_LIBRARY_SOURCE \${STANDARD_LIBRARY_SOURCE})
  file(APPEND \${STANDARD_LIBRARY_CPP} \${STANDARD_LIBRARY_SOURCE})
  file(APPEND \${STANDARD_LIBRARY_CPP} \"\\\";\\n}\\n\")
endif()")
include(${STANDARD_LIBRARY_CMAKE}) # Run it once
add_custom_target(standard_library COMMAND ${CMAKE_COMMAND} -P ${STANDARD_LIBRARY_CMAKE})

if(CMAKE_COMPILER_IS_GNUCC)
  set(CXX11 "-std=c++0x")
else()
  set(CXX11 "-std=c++11")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11} -Wall -Wextra -Wno-unused-parameter -fno-rtti")
add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)
add_library(likely SHARED likely.cpp standard.likely opencv.shim ${STANDARD_LIBRARY_CPP})
add_dependencies(likely standard_library LLVM OpenCV lua)
set_target_properties(likely PROPERTIES
                      DEFINE_SYMBOL LIKELY_LIBRARY
                      VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                      SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
target_link_libraries(likely ${REQ_LLVM_LIBRARIES} liblua opencv_highgui)
install(TARGETS likely
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
add_custom_command(TARGET likely POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E touch ${STANDARD_LIBRARY_CPP})

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
  aux_source_directory(qcustomplot/src QCUSTOMPLOT_SRC)
  aux_source_directory(qcustomplot/src/plottables QCUSTOMPLOT_SRC_PLOTTABLES)
  aux_source_directory(qcustomplot/src/items QCUSTOMPLOT_SRC_ITEMS)
  aux_source_directory(qcustomplot/src/layoutelements QCUSTOMPLOT_SRC_LAYOUTELEMENTS)
  set(QCUSTOMPLOT_SRC ${QCUSTOMPLOT_SRC} ${QCUSTOMPLOT_SRC_PLOTTABLES} ${QCUSTOMPLOT_SRC_ITEMS} ${QCUSTOMPLOT_SRC_LAYOUTELEMENTS})

  qt5_add_resources(RESOURCES img/img.qrc)
  add_executable(Dream dream.cpp ${QCUSTOMPLOT_SRC} ${RESOURCES})
  qt5_use_modules(Dream Quick Widgets PrintSupport)
  target_link_libraries(Dream likely)
  install(TARGETS Dream RUNTIME DESTINATION bin)
endif()

# Install
install(FILES likely.h DESTINATION include)
install(DIRECTORY share DESTINATION .)

# Package
include(CPack)
