set(LIKELY_SRC assumption_substitution.cpp
               axis_substitution.cpp
               backend.cpp
               frontend.cpp
               io.cpp
               loop_collapse.cpp
               memory_management.cpp)
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
                ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello-world-compiled.lisp
                ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello-world-interpreted.lisp
                ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello-world-main.lisp
                ${CMAKE_SOURCE_DIR}/share/likely/hello_world/hello-world-trained.lisp
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

# Build CUDA backend if possible
option(LIKELY_CUDA "Build Likely with (incomplete) CUDA backend" OFF)
if(${LIKELY_CUDA})
  find_library(CUDA_LIBRARY cuda)
  find_path(CUDA_INCLUDE cuda.h)
  include_directories(${CUDA_INCLUDE})
  set(LIKELY_SRC ${LIKELY_SRC} cuda.cpp)
  set(LIKELY_LIBS ${LIKELY_LIBS} ${CUDA_LIBRARY})
else()
  add_definitions(-DLIKELY_NO_COPROCESSOR)
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
         COMMAND benchmark -test -m)

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
           COMMAND likely -ast -c "${EXPR}" -e "${VALUE}")
endfunction()

function(test_expr EXPR VALUE)
  string(REGEX REPLACE "[\r\n\t ]+" " " CLEAN_EXPR "${EXPR}") # Remove extra white space for cleanliness
  string(MAKE_C_IDENTIFIER "${CLEAN_EXPR}" NAME)
  add_test(NAME "${NAME}"
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND likely -c "${CLEAN_EXPR}" -e "${VALUE}")
endfunction()

# List in the order they occur in standard.tex
test_ast("(this is some code) ; this is a comment" "(this is some code)")
test_ast("; (also a comment)" "")
test_ast("(back to code again)" "(back to code again)")
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
test_ast("{ x y z }" "({ x y z })")
test_ast("[ q ]" "([ q ])")
test_expr("42" 42)
#test_expr("-42" -42)
test_expr("0x2a" 42)
test_expr("1:+ 2" 3)
test_expr("3.sq:+ 4.sq" 25)
test_expr("1" 1)
test_expr("1.0" 1.0)
test_expr("1000000000000" "1000000000000")
test_expr("(0 1 2)" "(0 1 2)")
test_expr("(0.f32 1 2)" "(0.0 1.0 2.0)")
test_expr("(+ 2 2)" 4)
test_expr("(+ 1.8 2)" 3.8)
test_expr("(+ 1.8 2.2)" 4.0)
test_expr("(- 3 2)" 1)
test_expr("(- 2 3)" -1)
test_expr("(- 3.2 2)" 1.2)
test_expr("(- 3.2 2.2)" 1.0)
test_expr("(- 1)" -1)
test_expr("(- -1.1)" 1.1)
test_expr("(* 1 2)" 2)
test_expr("(* 1.3 2)" 2.6)
test_expr("(* 1.5 2.0)" 3.0)
test_expr("(/ 4 2)" 2)
test_expr("(/ 4.5 2)" 2.25)
test_expr("(/ 4.2 2.1)" 2.0)
test_expr("(% 7 3)" 1)
test_expr("(% 6 3)" 0)
test_expr("(% 6 3.0)" 0.0)
test_expr("(% 6.5 3)" 0.5)
test_expr("(% -6.5 3)" -0.5)
test_expr("(% 6.5 -3)" 0.5)
test_expr("(% -6.5 -3)" -0.5)
test_expr("(% 6.5 7.5)" 6.5)
test_expr("(% 6.5 3.25)" 0.0)
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
test_expr("(~ 0)" 2147483647)
test_expr("(~ 2147483647)" 0)
test_expr("(& 1 2);" 0)
test_expr("(& 1 3);" 1)
test_expr("(| 1 2);;" 3)
test_expr("(| 1 3);;" 3)
test_expr("(^ 1 2);;;" 3)
test_expr("(^ 1 3);;;" 2)
test_expr("(<< 2 0)" 2)
test_expr("(<< 2 1)" 4)
test_expr("(>> 2 0) " 2)
test_expr("(>> 2 1) " 1)
test_expr("(>> 2 2) " 0)
test_expr("(>> -2 0)" -2)
test_expr("(>> -2 1)" -1)
test_expr("(sqrt 2.0)" 1.41421)
test_expr("(sqrt 4)" 2.0)
test_expr("(sqrt 0)" 0.0)
test_expr("(sin 0)" 0.0)
test_expr("(sin 1.570796)" 1.0)
test_expr("(sin -1.570796)" -1.0)
test_expr("(sin 0.523599)" 0.5)
test_expr("(cos 0)" 1.0)
test_expr("(cos 3.141593)" -1.0)
test_expr("(cos 1.047198)" 0.5)
test_expr("(pow 2 3)" 8.0)
test_expr("(pow 2 -3)" 0.125)
test_expr("(pow -2 3)" -8.0)
test_expr("(pow 1.5 0.5)" 1.22474)
test_expr("(pow 2 0.5)" 1.41421)
test_expr("(pow 4 0.5)" 2.0)
test_expr("(pow 4 0)" 1.0)
test_expr("(exp 0)" 1.0)
test_expr("(exp 1)" 2.71828)
test_expr("(exp 1.5)" 4.48169)
test_expr("(exp2 0)" 1.0)
test_expr("(exp2 1)" 2.0)
test_expr("(exp2 0.5)" 1.41421)
test_expr("(exp2 3)" 8.0)
test_expr("(log 1)" 0.0)
test_expr("(log 2.718281)" 1.0)
test_expr("(log 7.389056)" 2.0)
test_expr("(log 0.5)" -0.693147)
test_expr("(log10 1)" 0.0)
test_expr("(log10 10)" 1.0)
test_expr("(log10 100)" 2.0)
test_expr("(log10 0.5)" -0.30103)
test_expr("(log2 1)" 0.0)
test_expr("(log2 2)" 1.0)
test_expr("(log2 4)" 2.0)
test_expr("(log2 0.5)" -1.0)
test_expr("(log2 10)" 3.32193)
test_expr("(copysign 3 -1.1)" -3.0)
test_expr("(copysign -4.3 2)" 4.3)
test_expr("(floor 2.3)" 2.0)
test_expr("(floor 3.8)" 3.0)
test_expr("(floor 5.5)" 5.0)
test_expr("(floor -2.3)" -3.0)
test_expr("(floor -3.8)" -4.0)
test_expr("(floor -5.5)" -6.0)
test_expr("(ceil 2.3)" 3.0)
test_expr("(ceil 3.8)" 4.0)
test_expr("(ceil 5.5)" 6.0)
test_expr("(ceil -2.3)" -2.0)
test_expr("(ceil -3.8)" -3.0)
test_expr("(ceil -5.5)" -5.0)
test_expr("(trunc 2.3)" 2.0)
test_expr("(trunc 3.8)" 3.0)
test_expr("(trunc 5.5)" 5.0)
test_expr("(trunc -2.3)" -2.0)
test_expr("(trunc -3.8)" -3.0)
test_expr("(trunc -5.5)" -5.0)
test_expr("(round 2.3)" 2.0)
test_expr("(round 3.8)" 4.0)
test_expr("(round 5.5)" 6.0)
test_expr("(round -2.3)" -2.0)
test_expr("(round -3.8)" -4.0)
test_expr("(round -5.5)" -6.0)
test_expr("x := 1
           y := (+ x 1)
           x := 4
           y
           (+ x 1)" 25)
test_expr("{
             x := 1.$
             x
           }" 1)
test_expr("{
             y := ($ 3 2)
             (+ (y 0) (y 1))
           }" 6)
test_expr("{
             x := 1.$
             x :<- (+ x 1)
             x
           }" 2)
test_expr("{
             y := ($ 0 2)
             (y 0) :<- 1
             (y 1) :<- (+ (y 0) 1)
             (+ (y 0) (y 1))
           }" 3)
test_expr("x := 1
           y := 2
           {
             x := 4
             y := 5
             (+ x y)
           }
           (+ x y)" 93)
test_expr("x := [ (sqrt (+ (* 3 3) (* 4 4))) ] x" 5.0)
test_expr("x := 1
           y := 2
           (? (< x y) x y)" 1)
test_expr("{
             x := 0.$
             (? (== x 0) (<- x 1))
             x
           }" 1)
test_expr("x := 0
           (? x 1 2)" 2)
test_expr("{
             end := 10
             i := 1.$
             j := 0.$
             loop := #
             (< i end) :?
             {
               j :<- (+ j i)
               i :<- (+ i 1)
               loop
             }
             j
           }" 45)
test_expr("((type 2.0) 1)" 1.0)
test_expr("((make-type 288) 1)" 1.0)
test_expr("(numeric-limit u8 1)" 255)
test_expr("(numeric-limit u8 0)" 0)
test_expr("(numeric-limit i8 1)" 127)
test_expr("(numeric-limit i8 0)" -128)
test_expr("(numeric-limit u32 1)" 4294967295)
test_expr("(numeric-limit u32 0)" 0)
test_expr("(numeric-limit i32 1)" 2147483647)
test_expr("(numeric-limit i32 0)" -2147483648)
test_expr("(numeric-limit f32 1)" 3.40282e+38)
test_expr("(numeric-limit f32 0)" -3.40282e+38)
test_expr("(channels (read-image \"data/misc/lenna.tiff\"))" 3)
test_expr("(columns (read-image \"data/misc/lenna.tiff\"))" 512)
test_expr("(rows (read-image \"data/misc/lenna.tiff\"))" 512)
test_expr("(frames (read-image \"data/misc/lenna.tiff\"))" 1)
test_expr("increment := (-> x (+ x 1))
           (increment 1)" 2)
test_expr("max := (-> (x y) (? (> x y) x y))
           (max 2 1)" 2)
test_expr("two := (-> () 2)
           (two)" 2)
test_expr("(try [ (+ 1 1) ] 3)" 2)
test_expr("(try [ (+ 1) ] 3)" 3)
test_expr("((extern int \"abs\" int) -4)" 4)
test_expr("my-abs := (-> x (? (>= x 0) x (- x)))
           (extern int \"my_abs\" int my-abs)" "")
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
test_expr("\"library/literate_programming.md\".import" 42)
test_expr("(cast 2.3 2)" 2)
test_expr("(cast 2.3 2.f32)" 2.3)
test_expr("(+ (u8.depth-double 255) 1.u8)" 256)
test_expr("(+ (u8.depth-atleast-16 255) 1.u8)" 256)
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
test_expr("{ j := 0.$ (iter (-> i (<- j (+ j i))) 10) j }" 45)
test_expr("\"data/misc/lenna.tiff\".read-image.min-element" 3)
test_expr("\"data/misc/lenna.tiff\".read-image.max-element" 255)

# Test other library files
function(test_file FILE VALUE)
  add_test(NAME ${FILE}
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
           COMMAND likely library/${FILE} -e "${VALUE}")
endfunction()

test_file(demos.md "")
test_file(export.md 3)
test_file(literate_programming.md 42)
test_file(literate_programming.tex 42)
test_file(matrix_io.lisp "\"Compression Ratio: \"7.31714")

# Generate IR for bechmarks
set(LIKELY_BENCHMARKS_DIR ${CMAKE_SOURCE_DIR}/ir/benchmarks)
function(add_benchmark RETVAL FILE PARAMETERS)
  foreach(KERNEL_TYPE "" "-m")
    string(MAKE_C_IDENTIFIER ${FILE} SYMBOL_NAME)
    string(MAKE_C_IDENTIFIER "${FILE} ${RETVAL} ${PARAMETERS}${KERNEL_TYPE}" FILE_NAME)
    set(IR_FILE ${LIKELY_BENCHMARKS_DIR}/${FILE_NAME}.ll)
    add_custom_command(OUTPUT ${IR_FILE}
                       WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                       COMMAND likely -ro ${KERNEL_TYPE} -c "(import \"library/${FILE}.md\") (extern ${RETVAL} \"${SYMBOL_NAME}\" ${PARAMETERS} ${FILE})" ${IR_FILE}
                       DEPENDS likely ${CMAKE_SOURCE_DIR}/library/${FILE}.md
                       VERBATIM)
    add_custom_target(${FILE_NAME} ALL
                      DEPENDS ${IR_FILE})
  endforeach()
endfunction()

if(NOT ${LIKELY_DEMOS} MATCHES "None")
  add_benchmark("u8SCXY"  "binary-threshold" "(u8SCXY u8S u8S)"   )
  add_benchmark("i16SCXY" "binary-threshold" "(i16SCXY i16S i16S)")
  add_benchmark("f32CXY"  "binary-threshold" "(f32CXY f32 f32)"   )
  add_benchmark("u8SXY"  "convert-grayscale" "u8SCXY")
  add_benchmark("u16SXY" "convert-grayscale" "u16SCXY")
  add_benchmark("f32XY"  "convert-grayscale" "f32CXY")
  add_benchmark("u8SCXY"  "multiply-add" "(u8SCXY f32 f32)" )
  add_benchmark("i16SCXY" "multiply-add" "(i16SCXY f32 f32)")
  add_benchmark("i32CXY"  "multiply-add" "(i32CXY f32 f32)" )
  add_benchmark("f32CXY"  "multiply-add" "(f32CXY f32 f32)" )
  add_benchmark("f64CXY"  "multiply-add" "(f64CXY f64 f64)" )
  add_benchmark("f64CXY" "min-max-loc" "u8SCXY" )
  add_benchmark("f64CXY" "min-max-loc" "i16SCXY")
  add_benchmark("f64CXY" "min-max-loc" "i32CXY" )
  add_benchmark("f64CXY" "min-max-loc" "f32CXY" )
  add_benchmark("f64CXY" "min-max-loc" "f64CXY" )
  add_benchmark("f32CXY" "normalize-l2" "f32CXY")
  add_benchmark("f64CXY" "normalize-l2" "f64CXY")
  add_benchmark("f32XY" "matrix-multiplication" "(f32XY f32XY)")
  add_benchmark("f64XY" "matrix-multiplication" "(f64XY f64XY)")
  add_benchmark("f32XY" "gemm" "(f32XY f32XY f64 f32XY f64)")
  add_benchmark("f64XY" "gemm" "(f64XY f64XY f64 f32XY f64)")
  add_benchmark("f32XY" "match-template" "(f32XY f32XY)")
  add_benchmark("f32X" "average" "u8XY")
  add_benchmark("f32X" "average" "i16XY")
  add_benchmark("f32X" "average" "f32XY")
  add_benchmark("f64X" "average" "f64XY")
  add_benchmark("f32XY" "multiply-transposed" "(u8XY f32X)")
  add_benchmark("f32XY" "multiply-transposed" "(i16XY f32X)")
  add_benchmark("f32XY" "multiply-transposed" "(f32XY f32X)")
  add_benchmark("f64XY" "multiply-transposed" "(f64XY f64X)")
  add_benchmark("f32XY" "covariance" "u8XY")
  add_benchmark("f32XY" "covariance" "i16XY")
  add_benchmark("f32XY" "covariance" "f32XY")
  add_benchmark("f64XY" "covariance" "f64XY")
endif()

# Compile and test demos
set(LIKELY_DEMOS_DIR ${CMAKE_SOURCE_DIR}/ir/demos)
function(add_demo FILE ARGS)
  foreach(KERNEL_TYPE "" "-m")
    set(IR_FILE ${LIKELY_DEMOS_DIR}/${FILE}${KERNEL_TYPE}.ll)
    set(OBJECT_FILE ${CMAKE_BINARY_DIR}/${FILE}${KERNEL_TYPE}${CMAKE_C_OUTPUT_EXTENSION})
    add_custom_command(OUTPUT ${IR_FILE}
                       WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                       COMMAND likely -ro ${KERNEL_TYPE} -q "library/${FILE}.md" ${IR_FILE}
                       DEPENDS likely ${CMAKE_SOURCE_DIR}/library/${FILE}.md
                       VERBATIM)
    add_custom_command(OUTPUT ${OBJECT_FILE}
                       COMMAND llc -O3 -filetype=obj ${IR_FILE} -o ${OBJECT_FILE}
                       DEPENDS llc ${IR_FILE})
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

set(LIKELY_DEMOS "Default" CACHE STRING "Compile and test demos.")
set_property(CACHE LIKELY_DEMOS PROPERTY STRINGS "None" "Default" "All")
if(NOT ${LIKELY_DEMOS} MATCHES "None")
  add_demo(mandelbrot_set "600 400 -2.f32 -1.f32 3.f32 2.f32 20")
  add_demo(gabor_wavelet "192 192 64.f32 64.f32 0.f32 128.f32 0.f32")
endif()
if(${LIKELY_DEMOS} MATCHES "All")
  add_demo(average_face "")
endif()
