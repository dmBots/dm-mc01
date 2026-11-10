/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  Calls target system initialization then loop in main.
  ******************************************************************************
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "main.h"
#include "board.h"
#include "can_test.h"
#include "rs485_echo_test.h"
#include "uart_loopback_test.h"
#include "vbat_test.h"
#include "key_test.h"
#include "mx_usb_drd_fs.h"
#include "mx_usbx_app.h"
#include "bsp_usb_cdc.h"
#include "factory_report.h"
#include "beep_music_test.h"
#include "imu_test.h"
#include "imu_heater.h"
#include "tim8_loopback_test.h"
#include "led_mode_test.h"
#include "lcd_test.h"

/*
 * 这是综合例程的入口，把通信、传感器、灯、蜂鸣器和屏幕演示放在一起运行。
 * 阅读顺序：先看本文件了解执行顺序，再看 app 各模块的头文件了解接线和现象。
 * board/bsp 是直接操作硬件的函数，例如开灯、发字节；app 调用这些函数完成具体演示。
 *
 * 常用函数的用法：
 *   init：开始时调用一次，完成设置或开始一轮测试。
 *   process：放在 while(1) 主循环中反复调用，每次推进一点工作，不是调用一次就会自动运行。
 *   HAL_GetTick()：取得上电后的毫秒数；1000 毫秒等于 1 秒，用来判断是否到了下一步的时间。
 *   get_summary：读取当前状态；函数返回 true 不等于测试通过，还要看结果中的 result。
 *
 * 运行前按 app 头文件说明接好 CAN、RS485、UART、TIM8 的测试线，先核对信号脚和电源脚。
 * 用 USB 数据线连接电脑，打开开发板的虚拟串口，选择 UTF-8 显示并勾选 DTR 打开信号。
 * 发送 REPORT 加回车可查看进度，等待最后一行中文结论；灯、声音、按键和屏幕效果还需实际观察。
 * 没接测试线时部分项目会失败，不能只据此判断开发板损坏。
 * USB 当前输出测试报告，不运行独立的 usb_test、sbus_test；LCD 在显示，但 LCD/SBUS 不计入报告。
 */
int main(void)
{
  /* 先让芯片的时钟、引脚和通信等硬件按工程配置工作；准备失败时不继续运行示例。
   * 初始化末尾还会调用 board.c 中的 post_system_init_hook()，检查并设置 BOOT_SEL=1。
   */
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    bool post_vbat_tests_started = false;
    vbat_test_summary_t vbat_summary;

    /* 打开外设 5 V 供电，先不打开加热供电；电压测试结束后才开始加热。 */
    board_init();

    /* 准备 USB 虚拟串口，电脑上的串口工具通过它读取测试报告。 */
    if(app_usbx_init()!=UX_SUCCESS)
      return -2;

    /* 开始各项演示。IMU 是惯性传感器，先读温度，等温度稳定后才统计运动数据。 */
    imu_test_init(HAL_GetTick()); /* 准备惯性传感器，并检查是否能正常通信。 */
    can_test_init(HAL_GetTick()); /* 检查 CAN 消息能否正确发送和接收。 */
    rs485_echo_test_init(HAL_GetTick()); /* 检查两路 RS485 能否收发并原样回送数据。 */
    uart_loopback_test_init(HAL_GetTick()); /* 检查串口发出的数据能否通过跳线回到自己。 */
    vbat_test_init(HAL_GetTick()); /* 多次读取电池输入电压，检查数值和稳定性。 */
    key_test_init(HAL_GetTick()); /* 读取模拟按键的电压，供屏幕显示。 */
    tim8_loopback_test_init(HAL_GetTick()); /* 发出周期脉冲，再通过跳线统计收到多少次。 */
    led_mode_test_init(HAL_GetTick()); /* 让 LED 闪烁，并用用户按键切换速度。 */
    factory_report_init(HAL_GetTick()); /* 准备在 USB 串口上输出各项结果。 */
    lcd_test_init(HAL_GetTick()); /* 准备屏幕上的文字、图片和按键读数演示。 */
    while (1) 
    {
      /* USB 的底层通信和数据发送都要反复处理；只初始化、不执行下面的处理函数，报告发不出去。 */
      app_usbx_process();
      imu_test_process(HAL_GetTick());
      bsp_usb_cdc_process();
      lcd_test_process(HAL_GetTick());
      led_mode_test_process(HAL_GetTick());
      can_test_process(HAL_GetTick());
      rs485_echo_test_process(HAL_GetTick());
      uart_loopback_test_process(HAL_GetTick());
      vbat_test_process(HAL_GetTick());
      /* 先测电池电压，再开始加热和播放旋律；这里检查的是“测完”，不要求电压一定通过。
       * post_vbat_tests_started 用于记住已经启动过，避免主循环每次经过这里都重新开始。
       */
      if (!post_vbat_tests_started &&
          vbat_test_get_summary(&vbat_summary) &&
          ((vbat_summary.result == VBAT_TEST_RESULT_PASSED) ||
           (vbat_summary.result == VBAT_TEST_RESULT_FAILED)))
      {
        imu_heater_init(HAL_GetTick());
        beep_music_test_init(HAL_GetTick());
        post_vbat_tests_started = true;
      }
      if (post_vbat_tests_started)
      {
        imu_heater_process(HAL_GetTick());
        beep_music_test_process(HAL_GetTick());
      }
      key_test_process(HAL_GetTick());
      tim8_loopback_test_process(HAL_GetTick());
      /* 把各项最新进度汇总后发给电脑；是否完成以及是否通过，看报告最后一行。 */
      factory_report_process(HAL_GetTick());
    }
  }
}
