option(BUILD_TESTING "Benchmark Likely against OpenCV" OFF)
include(CTest)
enable_testing()

if(${BUILD_TESTING})
  # Find OpenCV
  ExternalProject_Add(
    OpenCV-2-4-3
    PREFIX OpenCV
    URL http://downloads.sourceforge.net/project/opencvlibrary/opencv-unix/2.4.3/OpenCV-2.4.3.tar.bz2
    URL_MD5 c0a5af4ff9d0d540684c0bf00ef35dbe)

  find_package(OpenCV REQUIRED)
endif()
