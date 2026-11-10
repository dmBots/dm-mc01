# file-format: 1.0.0
cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

include(cmake/target.cmake)
include(cmake/${TOOLCHAIN_FILE})

project(mc01_rc_all)
add_executable(mc01_rc_all)
include(cmake/files.cmake)
include(cmake/flags.cmake)
include(cmake/components.cmake)
add_subdirectory(middleware/bmi08x)
add_subdirectory(board)
add_subdirectory(app)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
)

target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC
)

target_link_options(${CMAKE_PROJECT_NAME} PUBLIC
)

target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC
)

target_link_libraries(${CMAKE_PROJECT_NAME}
  ${CMAKE_PROJECT_NAME}_board
  ${CMAKE_PROJECT_NAME}_app
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex
          $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
          $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/${CMAKE_PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary
          $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
          $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/${CMAKE_PROJECT_NAME}.bin
  COMMENT "Generating HEX and BIN firmware images"
  VERBATIM
)
