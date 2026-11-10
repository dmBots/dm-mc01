# file-format: 1.0.0
if(CMAKE_BUILD_TYPE MATCHES "^(debug|release)_GCC_STM32C593RGT6$")
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE main.c main.h)
endif()
