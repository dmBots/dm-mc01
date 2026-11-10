add_library(${CMAKE_PROJECT_NAME}_app INTERFACE)

target_include_directories(${CMAKE_PROJECT_NAME}_app INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(${CMAKE_PROJECT_NAME}_app INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/can_test.c
  ${CMAKE_CURRENT_LIST_DIR}/can_test.h
  ${CMAKE_CURRENT_LIST_DIR}/rs485_echo_test.c
  ${CMAKE_CURRENT_LIST_DIR}/rs485_echo_test.h
  ${CMAKE_CURRENT_LIST_DIR}/uart_loopback_test.c
  ${CMAKE_CURRENT_LIST_DIR}/uart_loopback_test.h
  ${CMAKE_CURRENT_LIST_DIR}/sbus_test.c
  ${CMAKE_CURRENT_LIST_DIR}/sbus_test.h
  ${CMAKE_CURRENT_LIST_DIR}/vbat_test.c
  ${CMAKE_CURRENT_LIST_DIR}/vbat_test.h
  ${CMAKE_CURRENT_LIST_DIR}/key_test.c
  ${CMAKE_CURRENT_LIST_DIR}/key_test.h
  ${CMAKE_CURRENT_LIST_DIR}/led_mode_test.c
  ${CMAKE_CURRENT_LIST_DIR}/led_mode_test.h
  ${CMAKE_CURRENT_LIST_DIR}/usb_test.c
  ${CMAKE_CURRENT_LIST_DIR}/usb_test.h
  ${CMAKE_CURRENT_LIST_DIR}/factory_report.c
  ${CMAKE_CURRENT_LIST_DIR}/factory_report.h
  ${CMAKE_CURRENT_LIST_DIR}/beep_music_test.c
  ${CMAKE_CURRENT_LIST_DIR}/beep_music_test.h
  ${CMAKE_CURRENT_LIST_DIR}/imu_test.c
  ${CMAKE_CURRENT_LIST_DIR}/imu_test.h
  ${CMAKE_CURRENT_LIST_DIR}/imu_heater.c
  ${CMAKE_CURRENT_LIST_DIR}/imu_heater.h
  ${CMAKE_CURRENT_LIST_DIR}/tim8_loopback_test.c
  ${CMAKE_CURRENT_LIST_DIR}/tim8_loopback_test.h
  ${CMAKE_CURRENT_LIST_DIR}/lcd.c
  ${CMAKE_CURRENT_LIST_DIR}/lcd.h
  ${CMAKE_CURRENT_LIST_DIR}/lcd_font.c
  ${CMAKE_CURRENT_LIST_DIR}/lcd_font.h
  ${CMAKE_CURRENT_LIST_DIR}/lcd_picture.c
  ${CMAKE_CURRENT_LIST_DIR}/lcd_picture.h
  ${CMAKE_CURRENT_LIST_DIR}/lcd_test.c
  ${CMAKE_CURRENT_LIST_DIR}/lcd_test.h
)

target_link_libraries(${CMAKE_PROJECT_NAME}_app INTERFACE
  ${CMAKE_PROJECT_NAME}_board_bsp
  ${CMAKE_PROJECT_NAME}_bmi08x
)
