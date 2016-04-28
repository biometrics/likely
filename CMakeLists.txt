project(likely)
cmake_minimum_required(VERSION 3.3.2)
include(ExternalProject)

# Build configuration
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 5)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if(NOT MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden")
endif()

# Find or build LLVM
option(LIKELY_SYSTEM_LLVM "Use system LLVM" OFF)
if(LIKELY_SYSTEM_LLVM)
  find_package(LLVM REQUIRED)
  include_directories(${LLVM_INCLUDE_DIRS})
  link_directories(${LLVM_LIBRARY_DIRS})
else()
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(LLVM_ENABLE_ASSERTIONS ON CACHE INTERNAL "")
  endif()
  set(LLVM_TARGETS_TO_BUILD "host" CACHE STRING "LLVM targets")
  add_subdirectory(llvm EXCLUDE_FROM_ALL)
  set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/llvm/share/llvm/cmake/")
  include(LLVMConfig)
  include_directories(${CMAKE_SOURCE_DIR}/llvm/include
                      ${CMAKE_BINARY_DIR}/llvm/include)
  link_directories(${LLVM_LIBRARY_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
endif()
llvm_map_components_to_libnames(LIKELY_LLVM_LIBRARIES bitwriter ipo mcjit native support vectorize)
set(LIKELY_LIBS ${LIKELY_LIBS} ${LIKELY_LLVM_LIBRARIES})

# Find or build OpenCV
option(LIKELY_SYSTEM_OPENCV "Use system OpenCV" OFF)
if(LIKELY_SYSTEM_OPENCV)
  find_package(OpenCV REQUIRED)
else()
  option(BUILD_WITH_STATIC_CRT "OpenCV static MSVC CRT" OFF)
  option(BUILD_PERF_TESTS "OpenCV performance tests" OFF)
  option(BUILD_TESTS "OpenCV accuracy & regression tests" OFF)
  option(ENABLE_FAST_MATH "OpenCV fast-math" ON)
  option(WITH_CUDA "OpenCV CUDA support" OFF)
  option(WITH_OPENEXR "OpenCV OpenEXR support" OFF)
  add_subdirectory(opencv EXCLUDE_FROM_ALL)
  include_directories(${CMAKE_SOURCE_DIR}/opencv/modules/core/include
                      ${CMAKE_SOURCE_DIR}/opencv/modules/hal/include
                      ${CMAKE_SOURCE_DIR}/opencv/modules/highgui/include
                      ${CMAKE_SOURCE_DIR}/opencv/modules/imgcodecs/include
                      ${CMAKE_SOURCE_DIR}/opencv/modules/imgproc/include
                      ${CMAKE_SOURCE_DIR}/opencv/modules/videoio/include)
endif()
set(LIKELY_LIBS ${LIKELY_LIBS} opencv_core opencv_highgui opencv_imgcodecs opencv_imgproc opencv_videoio)

# Compiler configuration
if(NOT MSVC)
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} -Wl,-x,-s,--gc-sections")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-x,-s,--gc-sections")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-dead_strip")
  endif()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -fno-rtti")
endif()

# Put artifacts in the build directory root
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Add dataset targets
function(download_demo_image NAME MD5)
  file(DOWNLOAD "https://github.com/biometrics/likely/releases/download/v0.1/${NAME}.jpg"
                "${CMAKE_SOURCE_DIR}/data/demo/${NAME}.jpg"
                EXPECTED_MD5 ${MD5})
endfunction()

download_demo_image("average_face"   505e06e358a13c7619ee996bf184caff)
download_demo_image("gabor_wavelet"  cf0c27b391516783f7973c83f3fd68f7)
download_demo_image("mandelbrot_set" 544dd8db0eaa75a2da846fa33445b5c9)

function(download_archive NAME MD5)
  ExternalProject_Add(${NAME}
                      URL "https://github.com/biometrics/likely/releases/download/v0.1/${NAME}.tar.gz"
                      URL_MD5 ${MD5}
                      SOURCE_DIR "${CMAKE_SOURCE_DIR}/data/${NAME}"
                      CONFIGURE_COMMAND ""
                      BUILD_COMMAND ""
                      INSTALL_COMMAND "")
endfunction()

download_archive("lfwa" 5d1d409b3b7217ec931facd89a542c89)
download_archive("misc" 0c306f262dc5a2a2108cd805c0c40a99)
download_archive("mnist" c341c72e5da5e91fd421560aa04de5a1)

# Set up testing
enable_testing()

# Build targets
include_directories(include)
add_subdirectory(src)
add_subdirectory(share/likely/hello_world)
add_subdirectory(share/likely/model_translation)

# Build docs
add_custom_target(doc)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_SOURCE_DIR}/src/Doxyfile.in Doxyfile)
  add_custom_target(likely_doxygen ${DOXYGEN_EXECUTABLE} Doxyfile)
  add_dependencies(doc likely_doxygen)
endif()

find_package(LATEX)
if(LATEX_COMPILER)
  include(UseLATEX/UseLATEX.cmake)
  file(GLOB LIKELY_TEX RELATIVE ${CMAKE_SOURCE_DIR} library/*.tex)
  foreach(TEX_FILE ${LIKELY_TEX})
    latex_get_filename_component(NAME "${TEX_FILE}" NAME_WE)
    set(LIKELY_BIBFILE "library/${NAME}.bib")
    set(LIKELY_INDEX "USE_INDEX")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${LIKELY_BIBFILE}")
      unset(LIKELY_BIBFILE)
      unset(LIKELY_INDEX)
    endif()
    add_latex_document(${TEX_FILE}
                       BIBFILES ${LIKELY_BIBFILE}
                       NO_DEFAULT
                       MANGLE_TARGET_NAMES
                       ${LIKELY_INDEX})
    add_dependencies(doc ${NAME}_pdf)
  endforeach()
endif()

# Install
install(DIRECTORY include share DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.txt ${CMAKE_SOURCE_DIR}/README.md DESTINATION share/likely)

# Package
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An embeddable just-in-time Lisp for image recognition and heterogeneous computing.")
set(CPACK_PACKAGE_FILE_NAME "Likely-${CMAKE_SYSTEM_NAME}")
set(CPACK_PACKAGE_NAME "Likely")
set(CPACK_PACKAGE_VENDOR "Joshua C. Klontz")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.txt)
include(CPack)
