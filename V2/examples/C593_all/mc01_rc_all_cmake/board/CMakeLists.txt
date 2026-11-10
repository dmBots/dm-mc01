add_library(${CMAKE_PROJECT_NAME}_board INTERFACE)

target_include_directories(${CMAKE_PROJECT_NAME}_board INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(${CMAKE_PROJECT_NAME}_board INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/board.c
  ${CMAKE_CURRENT_LIST_DIR}/board.h
  ${CMAKE_CURRENT_LIST_DIR}/board_config.h
)

add_subdirectory(bsp)

target_link_libraries(${CMAKE_PROJECT_NAME}_board INTERFACE
  ${CMAKE_PROJECT_NAME}_board_bsp
)
