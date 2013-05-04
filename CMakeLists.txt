project(likely)
cmake_minimum_required(VERSION 2.8.9)

# Configure
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CPACK_PACKAGE_NAME "Likely")
set(CPACK_PACKAGE_DESCRIPTION "Literate Kernel Library")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)

# C++11 Compiler flags
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
endif()

# External Project Configuration
include(ExternalProject)
set(EXTERNAL_PROJECT_PATH "${CMAKE_BINARY_DIR}/3rdparty")
set(EXTERNAL_PROJECT_ARGS -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS} -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} -DCMAKE_INSTALL_PREFIX=${EXTERNAL_PROJECT_PATH})

# Build LLVM
ExternalProject_Add(
  LLVM
  PREFIX LLVM
  URL http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz
  URL_MD5 71610289bbc819e3e15fdd562809a2d7
  CMAKE_ARGS ${EXTERNAL_PROJECT_ARGS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

# Build Likely
if(EXISTS "${EXTERNAL_PROJECT_PATH}/share/llvm/cmake/LLVMConfig.cmake")
  # Find LLVM
  set(CMAKE_MODULE_PATH "${EXTERNAL_PROJECT_PATH}/share/llvm/cmake/")
  include(LLVMConfig)
  include_directories(${LLVM_INCLUDE_DIRS})
  link_directories(${LLVM_LIBRARY_DIRS})
  add_definitions(${LLVM_DEFINITIONS})
  llvm_map_components_to_libraries(REQ_LLVM_LIBRARIES jit native vectorize)

  # Compile in index.html as a string
  set(LIKELY_INDEX_HTML ${CMAKE_BINARY_DIR}/likely_index_html.cpp)
  file(WRITE ${LIKELY_INDEX_HTML} "// Probably a bad idea to modify this automatically generated source file!\n\n")
  file(APPEND ${LIKELY_INDEX_HTML} "#include \"likely.h\"\n\nconst char *likely_index_html()\n{\n\treturn \"")
  file(READ index.html INDEX_HTML)
  string(REPLACE "\"" "\\\"" INDEX_HTML ${INDEX_HTML})
  string(REPLACE "\n" "\"\n\t\"" INDEX_HTML ${INDEX_HTML})
  file(APPEND ${LIKELY_INDEX_HTML} ${INDEX_HTML})
  file(APPEND ${LIKELY_INDEX_HTML} "\";\n}\n")

  if(CMAKE_COMPILER_IS_GNUCC)
    set(CXX11 "-std=c++0x")
  else()
    set(CXX11 "-std=c++11")
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11} -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden -fvisibility-inlines-hidden")
  add_library(likely SHARED likely.cpp ${LIKELY_INDEX_HTML})
  add_dependencies(likely LLVM)
  set_target_properties(likely PROPERTIES
                        DEFINE_SYMBOL LIKELY_LIBRARY
                        VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                        SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
  target_link_libraries(likely ${REQ_LLVM_LIBRARIES})
  install(TARGETS likely
          RUNTIME DESTINATION bin
          LIBRARY DESTINATION lib
          ARCHIVE DESTINATION lib)

  add_subdirectory(tests)
endif()

# Install
install(FILES likely.h DESTINATION include)
install(FILES LICENSE.txt LikelyConfig.cmake README.md DESTINATION .)

# Package
include(CPack)
