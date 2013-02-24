# ================================================================
#  The Likely CMake configuration file
#
#  Usage from an external project:
#    In your CMakeLists.txt, add these lines:
#
#    find_package(Likely REQUIRED)
#    target_link_libraries(MY_TARGET_NAME ${LIKELY_LIBS})
# ================================================================

get_filename_component(LIKELY_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include_directories(${LIKELY_DIR}/include)
link_directories(${LIKELY_DIR}/lib)
set(LIKELY_LIBS likely)
