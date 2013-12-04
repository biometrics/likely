project(likely)
cmake_minimum_required(VERSION 2.8.11)
cmake_policy(SET CMP0020 NEW)

# Configure
set(CPACK_PACKAGE_NAME "Likely")
set(CPACK_PACKAGE_DESCRIPTION "Literate Kernel Library")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
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

# C++11 standard library configuration
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
endif()

if(NOT VS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
endif()

# Build LLVM
set(LLVM_TARGETS_TO_BUILD "host" CACHE STRING "LLVM targets")
add_subdirectory(llvm EXCLUDE_FROM_ALL)
set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/llvm/share/llvm/cmake/")
include(LLVMConfig)
include_directories(${CMAKE_SOURCE_DIR}/llvm/include
                    ${CMAKE_BINARY_DIR}/llvm/include)
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libraries(REQ_LLVM_LIBRARIES mcjit native vectorize)

option(LIKELY_AUX "Build Likely with matrix I/O and debugging" ON)
if(${LIKELY_AUX})
  option(BUILD_WITH_STATIC_CRT "OpenCV static MSVC CRT" OFF)
  option(WITH_CUDA "OpenCV CUDA support" OFF)
  option(WITH_OPENEXR "OpenCV OpenEXR support" OFF)
  add_subdirectory(opencv EXCLUDE_FROM_ALL)
  include_directories(${CMAKE_SOURCE_DIR}/opencv/modules/core/include
                      ${CMAKE_SOURCE_DIR}/opencv/modules/highgui/include
                      ${CMAKE_SOURCE_DIR}/opencv/modules/imgproc/include)

  option(CURL_STATICLIB "CURL static library" ON)
  add_subdirectory(curl EXCLUDE_FROM_ALL)
  include_directories(${CMAKE_SOURCE_DIR}/curl/include
                      ${CMAKE_BINARY_DIR}/curl/include/curl)
endif()

option(LIKELY_SCRIPT "Build Likely with Lua frontend (requires LIKELY_AUX)" ON)
if(${LIKELY_SCRIPT})
  add_subdirectory(lua EXCLUDE_FROM_ALL)
  include_directories(lua/src ${CMAKE_BINARY_DIR}/lua)
endif()

include_directories(include share/likely)
add_subdirectory(src)

install(DIRECTORY share DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.txt DESTINATION share/likely)
include(CPack)
