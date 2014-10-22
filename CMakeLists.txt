project(likely)
cmake_minimum_required(VERSION 2.8.12)
cmake_policy(SET CMP0020 NEW)
include(ExternalProject)

# Configure
set(CPACK_PACKAGE_NAME "Likely")
set(CPACK_PACKAGE_DESCRIPTION "The Likely Programming Language")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 2)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Build configuration
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CLANG ON)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(GCC ON)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  set(ICC ON)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(VS ON)
endif()

if(NOT VS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
endif()

if(APPLE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=10.9")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.9")
endif()

# Build in Debug mode by default
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "One of: Debug, Release, RelWithDebInfo, MinSizeRel" FORCE)
endif()

# Find or build LLVM
option(LIKELY_SYSTEM_LLVM "Use system LLVM" OFF)
if(${LIKELY_SYSTEM_LLVM})
  find_package(LLVM REQUIRED)
  include_directories(${LLVM_INCLUDE_DIRS})
  link_directories(${LLVM_LIBRARY_DIRS})
else()
  string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
  if("${BUILD_TYPE}" STREQUAL "debug")
    set(LLVM_ENABLE_ASSERTIONS ON CACHE INTERNAL "")
  endif()
  set(LLVM_TARGETS_TO_BUILD "host" CACHE STRING "LLVM targets")
  if(NOT WIN32)
    option(LLVM_ENABLE_FFI "Use libffi to call external functions from the interpreter" ON)
  endif()
  add_subdirectory(llvm EXCLUDE_FROM_ALL)
  set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/llvm/share/llvm/cmake/")
  include(LLVMConfig)
  include_directories(${CMAKE_SOURCE_DIR}/llvm/include
                      ${CMAKE_BINARY_DIR}/llvm/include)
  link_directories(${LLVM_LIBRARY_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
endif()
llvm_map_components_to_libnames(LIKELY_LLVM_LIBRARIES bitwriter ipo interpreter mcjit native support vectorize)
set(LIKELY_LIBS ${LIKELY_LIBS} ${LIKELY_LLVM_LIBRARIES})

option(LIKELY_IO "Build Likely with matrix I/O" ON)
if(${LIKELY_IO})
  # Find or build OpenCV
  option(LIKELY_SYSTEM_OPENCV "Use system OpenCV" OFF)
  if(${LIKELY_SYSTEM_OPENCV})
    find_package(OpenCV REQUIRED)
    set(LIKELY_LIBS ${LIKELY_LIBS} ${OpenCV_LIBS})
  else()
    option(BUILD_WITH_STATIC_CRT "OpenCV static MSVC CRT" OFF)
    option(WITH_CUDA "OpenCV CUDA support" OFF)
    option(WITH_OPENEXR "OpenCV OpenEXR support" OFF)
    add_subdirectory(opencv EXCLUDE_FROM_ALL)
    include_directories(${CMAKE_SOURCE_DIR}/opencv/modules/core/include
                        ${CMAKE_SOURCE_DIR}/opencv/modules/highgui/include
                        ${CMAKE_SOURCE_DIR}/opencv/modules/imgproc/include)
    set(LIKELY_LIBS ${LIKELY_LIBS} opencv_highgui)
  endif()
endif()

# Compiler configuration
if(NOT VS)
  if(GCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} -Wl,-x,-s,--gc-sections")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-x,-s,--gc-sections")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-dead_strip")
  endif()

  set(WARNING_FLAGS "-Wall -Wextra -Wno-unused-parameter")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} -fno-rtti")
endif()

# Put artifacts in the build directory root
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Add dataset targets
file(DOWNLOAD https://github.com/biometrics/likely/releases/download/v0.1/average_face.jpg
              "${CMAKE_SOURCE_DIR}/data/demo/average_face.jpg"
              EXPECTED_MD5 1c4eda72fd695402ab7188d8bfdcaee0)

file(DOWNLOAD https://github.com/biometrics/likely/releases/download/v0.1/gabor_wavelet.jpg
              "${CMAKE_SOURCE_DIR}/data/demo/gabor_wavelet.jpg"
              EXPECTED_MD5 cf0c27b391516783f7973c83f3fd68f7)

file(DOWNLOAD https://github.com/biometrics/likely/releases/download/v0.1/hello_world.jpg
              "${CMAKE_SOURCE_DIR}/data/demo/hello_world.jpg"
              EXPECTED_MD5 87f9409a6c28428565835413a41cc9e3)

file(DOWNLOAD https://github.com/biometrics/likely/releases/download/v0.1/mandelbrot_set.jpg
              "${CMAKE_SOURCE_DIR}/data/demo/mandelbrot_set.jpg"
              EXPECTED_MD5 544dd8db0eaa75a2da846fa33445b5c9)

ExternalProject_Add(lfwa
                    URL https://github.com/biometrics/likely/releases/download/v0.1/lfwa.tar.gz
                    URL_MD5 5d1d409b3b7217ec931facd89a542c89
                    SOURCE_DIR "${CMAKE_SOURCE_DIR}/data/lfw2"
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ""
                    INSTALL_COMMAND "")

ExternalProject_Add(misc
                    URL https://github.com/biometrics/likely/releases/download/v0.1/misc.tar.gz
                    URL_MD5 c69b1a52908da7b105667b2d4180651b
                    SOURCE_DIR "${CMAKE_SOURCE_DIR}/data/misc"
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ""
                    INSTALL_COMMAND "")

# Set up testing
enable_testing()

# Build targets
include_directories(include)
add_subdirectory(src)
add_subdirectory(share/likely/hello_world)

install(DIRECTORY include share DESTINATION ${CMAKE_INSTALL_PREFIX} PATTERN "*.DS_Store" EXCLUDE)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.txt ${CMAKE_SOURCE_DIR}/README.md DESTINATION share/likely)
include(CPack)
