project(likely)
cmake_minimum_required(VERSION 2.8.9)

# Configure
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CPACK_PACKAGE_NAME "Likely")
set(CPACK_PACKAGE_DESCRIPTION "Literate Kernel Library")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)

# Find LLVM
include(ExternalProject)
ExternalProject_Add(
   LLVM-3-2
   PREFIX LLVM
   URL http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz
)

find_path(LLVM_DIR share/llvm/cmake/LLVMConfig.cmake)
mark_as_advanced(LLVM_DIR)
set(CMAKE_MODULE_PATH "${LLVM_DIR}/share/llvm/cmake")
include(LLVMConfig)

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libraries(REQ_LLVM_LIBRARIES jit native)

# Compiler flags
if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fvisibility=hidden")
endif()

# Compile in index.html as a string
set(LIKELY_INDEX_HTML ${CMAKE_BINARY_DIR}/likely_index_html.cpp)
file(WRITE ${LIKELY_INDEX_HTML} "// Probably a bad idea to modify this automatically generated source file!\n\n")
file(APPEND ${LIKELY_INDEX_HTML} "#include \"likely.h\"\n\nconst char *likely_index_html()\n{\n\treturn \"")
file(READ index.html INDEX_HTML)
string(REPLACE "\"" "\\\"" INDEX_HTML ${INDEX_HTML})
string(REPLACE "\n" "\"\n\t\"" INDEX_HTML ${INDEX_HTML})
file(APPEND ${LIKELY_INDEX_HTML} ${INDEX_HTML})
file(APPEND ${LIKELY_INDEX_HTML} "\";\n}\n")

# Build
add_library(likely likely.cpp ${LIKELY_INDEX_HTML})
set_target_properties(likely PROPERTIES
                             DEFINE_SYMBOL LIKELY_LIBRARY
                             VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                             SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
target_link_libraries(likely ${REQ_LLVM_LIBRARIES})

# Install
install(TARGETS likely
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES likely.h DESTINATION include)

# Package
include(CPack)
