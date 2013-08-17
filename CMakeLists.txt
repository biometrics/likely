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

# Build LLVM
add_subdirectory(llvm)
set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/llvm/share/llvm/cmake/")
include(LLVMConfig)
include_directories(${CMAKE_SOURCE_DIR}/llvm/include)
include_directories(${CMAKE_BINARY_DIR}/llvm/include)
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libraries(REQ_LLVM_LIBRARIES jit native vectorize)

# Build Lua
add_subdirectory(lua)
include_directories(lua/src ${CMAKE_BINARY_DIR}/lua)

# Compile in index.html as a string
set(LIKELY_STANDARD_LIBRARY ${CMAKE_BINARY_DIR}/likely_standard_library.cpp)
file(WRITE ${LIKELY_STANDARD_LIBRARY} "// Probably a bad idea to modify this automatically generated source file!\n\n")
file(APPEND ${LIKELY_STANDARD_LIBRARY} "#include \"likely.h\"\n\nconst char *likely_standard_library()\n{\n\treturn\n\t\"")
file(READ standard.likely STANDARD_LIBRARY_SOURCE)
string(REPLACE "\\" "\\\\" STANDARD_LIBRARY_SOURCE ${STANDARD_LIBRARY_SOURCE})
string(REPLACE "\"" "\\\"" STANDARD_LIBRARY_SOURCE ${STANDARD_LIBRARY_SOURCE})
string(REPLACE "\n" "\\n\"\n\t\"" STANDARD_LIBRARY_SOURCE ${STANDARD_LIBRARY_SOURCE})
file(APPEND ${LIKELY_STANDARD_LIBRARY} ${STANDARD_LIBRARY_SOURCE})
file(APPEND ${LIKELY_STANDARD_LIBRARY} "\";\n}\n")

if(CMAKE_COMPILER_IS_GNUCC)
  set(CXX11 "-std=c++0x")
else()
  set(CXX11 "-std=c++11")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11} -Wall -Wextra -Wno-unused-parameter -fvisibility=hidden -fvisibility-inlines-hidden")
add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)
add_library(likely SHARED likely.cpp index.html standard.likely ${LIKELY_STANDARD_LIBRARY})
add_dependencies(likely LLVM)
set_target_properties(likely PROPERTIES
                      DEFINE_SYMBOL LIKELY_LIBRARY
                      VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                      SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
target_link_libraries(likely ${REQ_LLVM_LIBRARIES} liblua)
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

# Create
find_package(Qt5Quick QUIET)
find_package(Qt5Widgets QUIET)
if(${Qt5Quick_FOUND} AND ${Qt5Widgets_FOUND})
  qt5_add_resources(RESOURCES img/img.qrc)
  add_executable(Dream dream.cpp ${RESOURCES})
  qt5_use_modules(Dream Quick Widgets)
  target_link_libraries(Dream likely)
  install(TARGETS Dream RUNTIME DESTINATION bin)
endif()

# Install
install(FILES likely.h DESTINATION include)
install(DIRECTORY share DESTINATION .)

# Package
include(CPack)
