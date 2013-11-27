if(${LIKELY_AUX} AND ${LIKELY_SCRIPT})
  # Benchmark
  add_executable(benchmark benchmark.cpp)
  add_dependencies(benchmark OpenCV)
  target_link_libraries(benchmark likely opencv_imgproc)
  install(TARGETS benchmark RUNTIME DESTINATION bin)

  # Dream
  find_package(Qt5Quick QUIET)
  find_package(Qt5Widgets QUIET)
  find_package(Qt5PrintSupport QUIET)
  if(${Qt5Quick_FOUND} AND ${Qt5Widgets_FOUND} AND ${Qt5PrintSupport_FOUND})
    # Get QCustomPlot source
    # aux_source_directory(qcustomplot/src QCUSTOMPLOT_SRC)
    # aux_source_directory(qcustomplot/src/plottables QCUSTOMPLOT_SRC_PLOTTABLES)
    # aux_source_directory(qcustomplot/src/items QCUSTOMPLOT_SRC_ITEMS)
    # aux_source_directory(qcustomplot/src/layoutelements QCUSTOMPLOT_SRC_LAYOUTELEMENTS)
    # set(QCUSTOMPLOT_SRC ${QCUSTOMPLOT_SRC} ${QCUSTOMPLOT_SRC_PLOTTABLES} ${QCUSTOMPLOT_SRC_ITEMS} ${QCUSTOMPLOT_SRC_LAYOUTELEMENTS})

    qt5_add_resources(RESOURCES ../img/img.qrc)
    add_executable(Dream dream.cpp ${QCUSTOMPLOT_SRC} ${RESOURCES})
    qt5_use_modules(Dream Quick Widgets PrintSupport)
    target_link_libraries(Dream likely)
    install(TARGETS Dream RUNTIME DESTINATION bin)
  endif()
endif()
