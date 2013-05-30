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

# Build Submodules
add_subdirectory(llvm)
if(${LIKELY_BENCHMARK})
  add_subdirectory(opencv)
endif()

# Find LLVM
set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/llvm/share/llvm/cmake/")
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
add_library(likely SHARED likely.cpp index.html ${LIKELY_INDEX_HTML})
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

# Benchmark
find_package(OpenCV QUIET)
if(${OpenCV_FOUND})
  add_executable(benchmark benchmark.cpp)
  target_link_libraries(benchmark likely opencv_core opencv_imgproc)
  install(TARGETS benchmark RUNTIME DESTINATION bin)
endif()

# Install
install(FILES likely.h DESTINATION include)
install(DIRECTORY share DESTINATION .)

# Package
include(CPack)
