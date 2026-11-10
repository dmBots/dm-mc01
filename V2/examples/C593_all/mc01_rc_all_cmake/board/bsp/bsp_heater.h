/*
 * 控制 IMU（惯性传感器）加热器的输出，不在这里决定目标温度。
 * PWM 是周期性开关信号，占空比表示一个周期内输出有效信号的时间比例。
 * 入门使用直接看 app/imu_heater.h，由它自动读温度、调整加热比例和控制加热电源。
 */
#ifndef BSP_HEATER_H
#define BSP_HEATER_H

#include <stdbool.h>
#include <stdint.h>

/* init 后输出比例为 0%，还不会加热；再用 set_duty_per_mille 设置加热比例。 */
bool bsp_heater_init(void);
/* 用千分比设置比例：0=0%，500=50%，800=80%，1000=100%。
 * 这里只检查 0~1000；自动温控还会按 board_config.h 的上限限制输出。
 */
bool bsp_heater_set_duty_per_mille(uint16_t duty_per_mille);
/* 停止加热控制信号，不等于关闭加热支路电源；关电源还需 bsp_pwr_heat_5v_cmd(false)。 */
bool bsp_heater_shutdown(void);

#endif
