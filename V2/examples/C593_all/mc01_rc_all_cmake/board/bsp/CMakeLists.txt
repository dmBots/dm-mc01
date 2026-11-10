add_library(${CMAKE_PROJECT_NAME}_board_bsp INTERFACE)

target_include_directories(${CMAKE_PROJECT_NAME}_board_bsp INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/..
)

target_sources(${CMAKE_PROJECT_NAME}_board_bsp INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/bsp_led.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_led.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_user_key.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_user_key.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_pwr.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_pwr.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_canfd.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_canfd.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_rs485.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_rs485.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_uart.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_uart.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_uart_internal.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_sbus.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_sbus.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_sbus_internal.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_adc.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_adc.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_usb_cdc.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_usb_cdc.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_beep.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_beep.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_spi2.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_spi2.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_lcd.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_lcd.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_heater.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_heater.h
  ${CMAKE_CURRENT_LIST_DIR}/bsp_tim8_pwm_capture.c
  ${CMAKE_CURRENT_LIST_DIR}/bsp_tim8_pwm_capture.h
)
