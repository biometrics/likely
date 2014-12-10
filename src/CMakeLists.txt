set(LIKELY_SRC backend.cpp frontend.cpp io.cpp)
file(GLOB LIKELY_LIBRARY ${CMAKE_SOURCE_DIR}/library/*)
set(LIKELY_MISC ${LIKELY_LIBRARY}
                ${CMAKE_SOURCE_DIR}/LICENSE.txt
                ${CMAKE_SOURCE_DIR}/README.md
                ${CMAKE_SOURCE_DIR}/include/likely.h
                ${CMAKE_SOURCE_DIR}/include/likely/backend.h
                ${CMAKE_SOURCE_DIR}/include/likely/export.h
                ${CMAKE_SOURCE_DIR}/include/likely/frontend.h
                ${CMAKE_SOURCE_DIR}/include/likely/io.h
                ${CMAKE_SOURCE_DIR}/include/likely/opencv.hpp
                ${CMAKE_SOURCE_DIR}/include/likely/runtime.h
                ${CMAKE_SOURCE_DIR}/index.html
                ${CMAKE_SOURCE_DIR}/share/likely/LikelyConfig.cmake
                ${CMAKE_SOURCE_DIR}/share/likely/hello_world/README.md
                ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello_world_static.lisp
                ${CMAKE_SOURCE_DIR}/src/Doxyfile.in)

# Build standard library
add_executable(embedfile embedfile.c)
add_custom_command(OUTPUT likely_standard_library.c
                   COMMAND embedfile likely_standard_library ${CMAKE_SOURCE_DIR}/library/standard.tex
                   DEPENDS embedfile ${CMAKE_SOURCE_DIR}/library/standard.tex)
set(LIKELY_SRC ${LIKELY_SRC} ${CMAKE_CURRENT_BINARY_DIR}/likely_standard_library.c)
set(LIKELY_DEPENDS ${LIKELY_DEPENDS} likely_standard_library.c)

# CPU Profiling
option(LIKELY_PROFILING "Build Likely for code profiling" OFF)
if(${LIKELY_PROFILING})
  if(NOT VS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  endif()
endif()

# Build runtime-only library
add_definitions(-DLIKELY_LIBRARY)
add_library(likely_runtime STATIC runtime.c runtime_stdthread.cpp)

# Build common object files
add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)
add_library(likely_object OBJECT ${LIKELY_SRC})
add_dependencies(likely_object likely_runtime ${LIKELY_LIBS} ${LIKELY_DEPENDS})

# Build shared library
add_library(likely_shared SHARED $<TARGET_OBJECTS:likely_object> ${LIKELY_MISC})
set_target_properties(likely_shared PROPERTIES OUTPUT_NAME likely
                                               VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
                                               SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
target_link_libraries(likely_shared PRIVATE likely_runtime ${LIKELY_LIBS})
install(TARGETS likely_shared ARCHIVE DESTINATION lib
                              LIBRARY DESTINATION lib
                              RUNTIME DESTINATION bin)

# Build static library
add_library(likely_static STATIC $<TARGET_OBJECTS:likely_object>)
if(VS)
  set_target_properties(likely_static PROPERTIES OUTPUT_NAME liblikely) # Avoid name collision
else()
  set_target_properties(likely_static PROPERTIES OUTPUT_NAME likely)
endif()
target_link_libraries(likely_static likely_runtime ${LIKELY_LIBS})
install(TARGETS likely_static ARCHIVE DESTINATION lib)

# Build console-interpreter-compiler
add_executable(likely likely.cpp)
set_target_properties(likely PROPERTIES OUTPUT_NAME likely)
target_link_libraries(likely likely_shared LLVMSupport)
install(TARGETS likely RUNTIME DESTINATION bin)

# Benchmark
add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark likely_shared LLVMSupport opencv_imgproc opencv_highgui)
install(TARGETS benchmark RUNTIME DESTINATION bin)
add_test(NAME benchmark
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
         COMMAND benchmark -test)
add_test(NAME benchmark_parallel
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
         COMMAND benchmark -test -parallel)

# Main
add_library(likely_main OBJECT main.c)

# Dream
set(QT5_INSTALL_DIR "${CMAKE_BINARY_DIR}/Qt5.3.2")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT5_INSTALL_DIR})
find_package(Qt5OpenGL QUIET)
find_package(Qt5PrintSupport QUIET)
find_package(Qt5Widgets QUIET)
set(CMAKE_AUTOMOC ON)
if(${Qt5OpenGL_FOUND} AND ${Qt5PrintSupport_FOUND} AND ${Qt5Widgets_FOUND})
  # Get QCustomPlot source
  # aux_source_directory(qcustomplot/src QCUSTOMPLOT_SRC)
  # aux_source_directory(qcustomplot/src/plottables QCUSTOMPLOT_SRC_PLOTTABLES)
  # aux_source_directory(qcustomplot/src/items QCUSTOMPLOT_SRC_ITEMS)
  # aux_source_directory(qcustomplot/src/layoutelements QCUSTOMPLOT_SRC_LAYOUTELEMENTS)
  # set(QCUSTOMPLOT_SRC ${QCUSTOMPLOT_SRC} ${QCUSTOMPLOT_SRC_PLOTTABLES} ${QCUSTOMPLOT_SRC_ITEMS} ${QCUSTOMPLOT_SRC_LAYOUTELEMENTS})

  if(QT_STATIC_DEPENDENCIES)
    add_definitions(-DLIKELY_STATIC_QT)
  endif()
  add_executable(dream dream.cpp ${QCUSTOMPLOT_SRC})
  qt5_use_modules(dream OpenGL PrintSupport Widgets)
  target_link_libraries(dream likely_shared LLVMSupport ${QT_STATIC_DEPENDENCIES})
  install(TARGETS dream RUNTIME DESTINATION bin)
else()
  if(WIN32)
    # TODO: https://bugreports.qt-project.org/browse/QTBUG-33450
    # ExternalProject_Add(Qt5.3.2
    #     URL http://download.qt-project.org/official_releases/qt/5.3/5.3.2/submodules/qtbase-opensource-src-5.3.2.zip
    #     URL_MD5 1a81aafd9d63168e9d0d1056ae0cc5d2
    #     CONFIGURE_COMMAND ../Qt5.3.2/configure.exe -prefix ${QT5_INSTALL_DIR} -opensource -confirm-license -static -release -no-compile-examples
    #     BUILD_COMMAND nmake
    #     INSTALL_COMMAND nmake install)
  else() # UNIX
    if(NOT APPLE)
      set(QT_CONFIGURE_FLAGS "-qt-xcb")
    endif()
    ExternalProject_Add(Qt5.3.2
        URL http://download.qt-project.org/official_releases/qt/5.3/5.3.2/submodules/qtbase-opensource-src-5.3.2.tar.gz
        URL_MD5 2a692eed83c4f7e48e1db65ed1a2ae9f
        CONFIGURE_COMMAND ../Qt5.3.2/configure -prefix ${QT5_INSTALL_DIR} -opensource -confirm-license -static -release -no-compile-examples ${QT_CONFIGURE_FLAGS})
    if(APPLE)
      find_package(OpenGL REQUIRED)
      find_library(COCOA_LIBRARY Cocoa)
      find_library(CARBON_LIBRARY Carbon)
      find_library(CUPS_LIBRARY cups)
      find_library(IOKIT_LIBRARY IOKit)
      set(QT_STATIC_DEPENDENCIES ${OPENGL_LIBRARIES}
                                 ${COCOA_LIBRARY}
                                 ${CARBON_LIBRARY}
                                 ${CUPS_LIBRARY}
                                 ${IOKIT_LIBRARY}
                                 ${QT5_INSTALL_DIR}/lib/libQt5PlatformSupport.a
                                 ${QT5_INSTALL_DIR}/lib/libqtharfbuzzng.a
                                 ${QT5_INSTALL_DIR}/plugins/platforms/libqcocoa.a
          CACHE INTERNAL "Qt static dependencies")
    else()
      find_package(OpenGL REQUIRED)
      find_library(XCB_LIBRARY xcb)
      find_library(X11_LIBRARY X11)
      find_library(X11XCB_LIBRARY X11-xcb)
      find_library(Z_LIBRARY z)
      set(QT_STATIC_DEPENDENCIES ${OPENGL_LIBRARIES}
                                 ${QT5_INSTALL_DIR}/lib/libQt5PlatformSupport.a
                                 ${QT5_INSTALL_DIR}/plugins/platforms/libqxcb.a
                                 ${QT5_INSTALL_DIR}/lib/libxcb-static.a
                                 ${QT5_INSTALL_DIR}/lib/libQt5Gui.a # Referenced by Qt5PlatformSupport
                                 ${XCB_LIBRARY}
                                 ${X11_LIBRARY}
                                 ${X11XCB_LIBRARY}
                                 ${Z_LIBRARY}
          CACHE INTERNAL "Qt static dependencies")
    endif()
  endif()
endif()

# Test standard library
function(test_ast EXPR VALUE)
  string(MAKE_C_IDENTIFIER "${EXPR}" NAME)
  add_test(NAME "${NAME}"
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND likely -ast -command "${EXPR}" -assert "${VALUE}")
endfunction()

function(test_expr EXPR VALUE)
  string(MAKE_C_IDENTIFIER "${EXPR}" NAME)
  add_test(NAME "${NAME}"
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND likely -command "${EXPR}" -assert "${VALUE}")
endfunction()

# List in the order they occur in standard.tex
test_ast("(this is some code) ; This is a comment" "(this is some code)")
test_expr("(+ 1 2) ; One plus two is three" 3)
test_expr("(sq 3)  ; Three squared is nine" 9)
test_ast("x.f" "(f x)")
test_ast("x.f.g" "(g (f x))")
test_ast("x.(f y)" "(f x y)")
test_ast("(f x).g" "(g (f x))")
test_ast("(g x.f)" "(g (f x))")
test_ast("7.2" "7.2")
test_expr("3.sq" 9)
test_expr("1.(+ 2)" 3)
test_ast("x:f y" "(f x y)")
test_ast("z:g x:f y " "(g z (f x y))")
test_ast("x:f (g y)" "(f x (g y))")
test_ast("x.f:h y.g" "(h (f x) (g y))")
test_ast("(g x:f y)" "(g (f x y))")
test_expr("1:+ 2" 3)
test_expr("3.sq:+ 4.sq" 25)
test_expr("(+ 2 2)" 4)
test_expr("(+ 1.8 2)" 3.8)
test_expr("(+ 1.8 2.1)" 3.9)
test_expr("(+ 1.8 2.2)" 4)
test_expr("(- 3 2)" 1)
test_expr("(- 2 3)" -1)
test_expr("(- 3.2 2)" 1.2)
test_expr("(- 3.2 2.1)" 1.1)
test_expr("(- 3.2 2.2)" 1)
test_expr("(- 1)" -1)
test_expr("(- -1.1)" 1.1)
test_expr("(* 1 2)" 2)
test_expr("(* 1.3 2)" 2.6)
test_expr("(* 1.4 2.1)" 2.94)
test_expr("(* 1.5 2.0)" 3)
test_expr("(/ 4 2)" 2)
test_expr("(/ 4.5 2)" 2.25)
test_expr("(/ 4.5 2.5)" 1.8)
test_expr("(/ 4.2 2.1)" 2)
test_expr("(% 7 3)" 1)
test_expr("(% 6 3)" 0)
test_expr("(% 6.5 3)" 0.5)
test_expr("(% -6.5 3)" -0.5)
test_expr("(% 6.5 -3)" 0.5)
test_expr("(% -6.5 -3)" -0.5)
test_expr("(% 6.5 7.5)" 6.5)
test_expr("(% 6.5 3.25)" 0)
test_expr("(== 2 2)" 1)
test_expr("(== 2 2.0)" 1)
test_expr("(== 2 -2)" 0)
test_expr("(== 2 2.1)" 0)
test_expr("(!= 3 3)" 0)
test_expr("(!= 3 3.0)" 0)
test_expr("(!= 3 -3)" 1)
test_expr("(!= 3 3.1)" 1)
test_expr("(< 4 5)" 1)
test_expr("(< 4 -5.0)" 0)
test_expr("(< 4 4.0)" 0)
test_expr("(<= 4 5)" 1)
test_expr("(<= 4 -5.0)" 0)
test_expr("(<= 4 4.0)" 1)
test_expr("(> 6 7)" 0)
test_expr("(> 6 -7.0)" 1)
test_expr("(> 6 6.0)" 0)
test_expr("(>= 6 7)" 0)
test_expr("(>= 6 -7.0)" 1)
test_expr("(>= 6 6.0)" 1)
test_expr("(& 1 2) " 0)
test_expr("(& 1 3) " 1)
test_expr("(| 1 2)  " 3)
test_expr("(| 1 3)  " 3)
test_expr("(^ 1 2)   " 3)
test_expr("(^ 1 3)   " 2)
test_expr("(<< 2 0)" 2)
test_expr("(<< 2 1)" 4)
test_expr("(>> 2 0) " 2)
test_expr("(>> 2 1) " 1)
test_expr("(>> 2 2) " 0)
test_expr("(>> -2 0)" -2)
test_expr("(>> -2 1)" -1)
test_expr("(sqrt 2)" 1.41421)
test_expr("(sqrt 2.1)" 1.44914)
test_expr("(sqrt 4)" 2)
test_expr("(sqrt 0)" 0)
test_expr("(sin 0)" 0)
test_expr("(sin 1.570796)" 1)
test_expr("(sin -1.570796)" -1)
test_expr("(sin 0.523599)" 0.5)
test_expr("(cos 0)" 1)
test_expr("(cos 3.141593)" -1)
test_expr("(cos 1.047198)" 0.5)
test_expr("(pow 2 3)" 8)
test_expr("(pow 2 -3)" 0.125)
test_expr("(pow -2 3)" -8)
test_expr("(pow 1.5 0.5)" 1.22474)
test_expr("(pow 2 0.5)" 1.41421)
test_expr("(pow 4 0.5)" 2)
test_expr("(pow 4 0)" 1)
test_expr("(exp 0)" 1)
test_expr("(exp 1)" 2.71828)
test_expr("(exp 1.5)" 4.48169)
test_expr("(exp2 0)" 1)
test_expr("(exp2 1)" 2)
test_expr("(exp2 0.5)" 1.41421)
test_expr("(exp2 3)" 8)
test_expr("(log 1)" 0)
test_expr("(log 2.718281)" 1)
test_expr("(log 7.389056)" 2)
test_expr("(log 0.5)" -0.693147)
test_expr("(log10 1)" 0)
test_expr("(log10 10)" 1)
test_expr("(log10 100)" 2)
test_expr("(log10 0.5)" -0.30103)
test_expr("(log2 1)" 0)
test_expr("(log2 2)" 1)
test_expr("(log2 4)" 2)
test_expr("(log2 0.5)" -1)
test_expr("(log2 10)" 3.32193)
test_expr("(copysign 3 -1.1)" -3)
test_expr("(copysign -4.3 2)" 4.3)
test_expr("(floor 2.3)" 2)
test_expr("(floor 3.8)" 3)
test_expr("(floor 5.5)" 5)
test_expr("(floor -2.3)" -3)
test_expr("(floor -3.8)" -4)
test_expr("(floor -5.5)" -6)
test_expr("(ceil 2.3)" 3)
test_expr("(ceil 3.8)" 4)
test_expr("(ceil 5.5)" 6)
test_expr("(ceil -2.3)" -2)
test_expr("(ceil -3.8)" -3)
test_expr("(ceil -5.5)" -5)
test_expr("(trunc 2.3)" 2)
test_expr("(trunc 3.8)" 3)
test_expr("(trunc 5.5)" 5)
test_expr("(trunc -2.3)" -2)
test_expr("(trunc -3.8)" -3)
test_expr("(trunc -5.5)" -5)
test_expr("(round 2.3)" 2)
test_expr("(round 3.8)" 4)
test_expr("(round 5.5)" 6)
test_expr("(round -2.3)" -2)
test_expr("(round -3.8)" -4)
test_expr("(round -5.5)" -6)
test_expr("(new u8CXY 3 512 512 1 null).channels" 3)
test_expr("(new u8CXY 3 512 512 1 null).columns" 512)
test_expr("(new u8CXY 3 512 512 1 null).rows" 512)
test_expr("(new u8CXY 3 512 512 1 null).frames" 1)
test_expr("(try (+ 1 1) 3)" 2)
test_expr("(try (+ 1) 3)" 3)
test_expr("((extern i32 \"abs\" i32) -4)" 4)
test_expr("true" 1)
test_expr("false" 0)
test_expr("e" 2.71828)
test_expr("pi" 3.14159)
test_expr("(not 0.0)" 1)
test_expr("(not 1.1)" 0)
test_expr("(bool 0.0)" 0)
test_expr("(bool 1.1)" 1)
test_expr("(sq -1.3)" 1.69)
test_expr("(abs -1.3)" 1.3)
test_expr("(and 0 1)" 0)
test_expr("(and 1 1)" 1)
test_expr("(or 0 1)" 1)
test_expr("(or 1 1)" 1)
test_expr("(xor 0 1)" 1)
test_expr("(xor 1 1)" 0)
test_expr("(min -1 1.3)" -1)
test_expr("(max 1 -1.3)" 1)
test_expr("(new u8CXY 3 512 512 1 null).elements" 786432)
test_expr("(new u8CXY 3 512 512 1 null).bytes" 786432)
test_expr("\"library/comments.md\".(import this)" 42)
test_expr("(cast 2.3 2)" 2)
test_expr("(cast 2.3 2.f32)" 2.3)
test_expr("(threshold-binary 0.4 0.5 1)" 0)
test_expr("(threshold-binary 0.5 0.5 1)" 0)
test_expr("(threshold-binary 0.6 0.5 1)" 1)
test_expr("(threshold-binary-inverse 0.4 0.5 1)" 1)
test_expr("(threshold-binary-inverse 0.5 0.5 1)" 1)
test_expr("(threshold-binary-inverse 0.6 0.5 1)" 0)
test_expr("(threshold-truncate 0.4 0.5)" 0.4)
test_expr("(threshold-truncate 0.5 0.5)" 0.5)
test_expr("(threshold-truncate 0.6 0.5)" 0.5)
test_expr("(threshold-to-zero 0.4 0.5)" 0)
test_expr("(threshold-to-zero 0.5 0.5)" 0)
test_expr("(threshold-to-zero 0.6 0.5)" 0.6)
test_expr("(threshold-to-zero-inverse 0.4 0.5)" 0.4)
test_expr("(threshold-to-zero-inverse 0.5 0.5)" 0.5)
test_expr("(threshold-to-zero-inverse 0.6 0.5)" 0)

# Test other library files
function(test_file FILE VALUE)
  add_test(NAME ${FILE}
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND likely library/${FILE}.md -assert "${VALUE}")
endfunction()

test_file(comments 42)
test_file(demos "")
test_file(export 3)
test_file(likely "")
test_file(matrix_io "Compression Ratio: 7.31714")
test_file(tutorial "")

# Compile and test demos
function(add_demo FILE RETURN PARAMETERS ARGS)
  foreach(KERNEL_TYPE "" "-p")
    set(BITCODE_FILE ${CMAKE_BINARY_DIR}/${FILE}${KERNEL_TYPE}.bc)
    set(IR_FILE ${CMAKE_BINARY_DIR}/${FILE}${KERNEL_TYPE}.ll)
    set(OBJECT_FILE ${CMAKE_BINARY_DIR}/${FILE}${KERNEL_TYPE}${CMAKE_C_OUTPUT_EXTENSION})
    add_custom_command(OUTPUT ${BITCODE_FILE}
                       WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                       COMMAND likely -Oz -disable-loop-unrolling -disable-loop-vectorization ${KERNEL_TYPE} -q -c "\"library/${FILE}.md\".(import this) (extern ${RETURN} \"likely_test_function\" ${PARAMETERS} ${FILE} true)" ${BITCODE_FILE}
                       DEPENDS likely ${CMAKE_SOURCE_DIR}/library/${FILE}.md
                       VERBATIM)
    add_custom_command(OUTPUT ${IR_FILE}
                       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                       COMMAND llvm-dis ${BITCODE_FILE}
                       DEPENDS llvm-dis ${BITCODE_FILE})
    add_custom_command(OUTPUT ${OBJECT_FILE}
                       COMMAND llc -O3 -filetype=obj ${BITCODE_FILE} -o ${OBJECT_FILE}
                       DEPENDS llc ${BITCODE_FILE})
    set(DEMO_EXECUTABLE ${FILE}${KERNEL_TYPE})
    add_executable(${DEMO_EXECUTABLE} $<TARGET_OBJECTS:likely_main> ${OBJECT_FILE} ${IR_FILE})
    set_source_files_properties(${OBJECT_FILE} PROPERTIES EXTERNAL_OBJECT true GENERATED true)
    target_link_libraries(${DEMO_EXECUTABLE} likely_shared)
    separate_arguments(ARGS)
    add_test(NAME ${FILE}${KERNEL_TYPE}
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
             COMMAND ${DEMO_EXECUTABLE} "\"data/demo/${FILE}.jpg\".read-image" ${ARGS})
  endforeach()
endfunction()

option(LIKELY_DEMOS "Compile and test simple demos." ON)
if(${LIKELY_DEMOS})
  add_demo(gabor_wavelet "f32XY" "(i32 i32 f32 f32 f32 f32 f32)" "192 192 64.f32 64.f32 0.f32 128.f32 0.f32")
  add_demo(hello_world "u8CXY" "u8CXY" "\"data/misc/lenna.tiff\".read-image")
  add_demo(mandelbrot_set "u8XY" "(i32 i32 f32 f32 f32 f32 i32)" "600 400 -2.f32 -1.f32 3.f32 2.f32 20")
endif()

# Compile and test algorithms
option(LIKELY_ALGORITHMS "Compile and test complex algorithms." OFF)
if(${LIKELY_ALGORITHMS})
  add_demo(average_face "u8XY" "()" "")
endif()
