project(likely)
cmake_minimum_required(VERSION 2.8.11)
cmake_policy(SET CMP0020 NEW)

# Configure
set(CPACK_PACKAGE_NAME "Likely")
set(CPACK_PACKAGE_DESCRIPTION "Likely Image Recognition Runtime")
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
  add_subdirectory(llvm EXCLUDE_FROM_ALL)
  set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/llvm/share/llvm/cmake/")
  include(LLVMConfig)
  include_directories(${CMAKE_SOURCE_DIR}/llvm/include
                      ${CMAKE_BINARY_DIR}/llvm/include)
  link_directories(${LLVM_LIBRARY_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
endif()
llvm_map_components_to_libraries(REQ_LLVM_LIBRARIES bitwriter ipo mcjit native vectorize)
set(LIKELY_LIBS ${LIKELY_LIBS} ${REQ_LLVM_LIBRARIES})

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

  add_subdirectory(libarchive EXCLUDE_FROM_ALL)
  include_directories(${CMAKE_SOURCE_DIR}/libarchive/libarchive)
  set(LIKELY_LIBS ${LIKELY_LIBS} archive_static)
  add_definitions(-DLIBARCHIVE_STATIC)

  # option(CURL_STATICLIB "CURL static library" ON)
  # add_subdirectory(curl EXCLUDE_FROM_ALL)
  # include_directories(${CMAKE_SOURCE_DIR}/curl/include
  #                     ${CMAKE_BINARY_DIR}/curl/include/curl)
endif()

# Compiler configuration
if(NOT VS)
  if(GCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()

  set(WARNING_FLAGS "-Wall -Wextra -Wno-unused-parameter")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} -fno-rtti")
  set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} -Wl,-x")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-x")
endif()

if(GCC)
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s")
endif()

if(APPLE)
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} -Wl,-dead_strip")
endif()

# Put artifacts in the build directory root
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Set up testing
enable_testing()
macro(likely_add_test executable)
  add_test(NAME ${executable}
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data
           COMMAND ${executable} ${ARGV1})
endmacro()

# Build targets
include_directories(include)
add_subdirectory(src)
add_subdirectory(share/likely/hello_world)

if(UNIX)
  execute_process(COMMAND cmake -E create_symlink ${CMAKE_SOURCE_DIR}/data ${CMAKE_BINARY_DIR}/data)
endif()

install(DIRECTORY include share DESTINATION ${CMAKE_INSTALL_PREFIX} PATTERN "*.DS_Store" EXCLUDE)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.txt ${CMAKE_SOURCE_DIR}/README.md DESTINATION share/likely)
include(CPack)
