# ============================================================================ #
#  The Likely CMake configuration file                                         #
#                                                                              #
#  Usage from an external project:                                             #
#                                                                              #
#    find_package(Likely REQUIRED)                                             #
#    target_link_libraries(MY_TARGET_NAME ${Likely_LIBS})                      #
# ============================================================================ #

include_directories(${Likely_DIR}/../../include)
link_directories(${Likely_DIR}/../../lib)
set(Likely_LIBS likely)
