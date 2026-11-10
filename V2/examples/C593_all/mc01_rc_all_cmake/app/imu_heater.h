/*
 * 自动给 IMU 惯性传感器加热，让运动数据在温度稳定后再开始测试。
 * PI 是这里使用的自动调温方法：根据当前温差和之前累计的温差，调整加热输出比例。
 * 目标温度、允许误差和等待时间在 board_config.h；温度数值 40000 表示 40 摄氏度。
 * 主循环先执行 imu_test_process 读取温度，再执行 imu_heater_process 调整加热。
 * “通过”后仍继续保持温度；温度不可用超时、过热或控制出错时，会停止加热并关闭加热电源。
 */
#ifndef IMU_HEATER_H
#define IMU_HEATER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    IMU_HEATER_RESULT_IDLE = 0U,
    IMU_HEATER_RESULT_RUNNING,
    IMU_HEATER_RESULT_PASSED,
    IMU_HEATER_RESULT_FAILED
} imu_heater_result_t;

typedef enum
{
    IMU_HEATER_STATE_WAIT_SENSOR = 0U,
    IMU_HEATER_STATE_WARMUP,
    IMU_HEATER_STATE_REGULATING,
    IMU_HEATER_STATE_STABLE,
    IMU_HEATER_STATE_FAULT
} imu_heater_state_t;

typedef enum
{
    IMU_HEATER_FAULT_NONE = 0U,
    IMU_HEATER_FAULT_BSP_INIT,
    IMU_HEATER_FAULT_SENSOR_TIMEOUT,
    IMU_HEATER_FAULT_OVER_TEMPERATURE,
    IMU_HEATER_FAULT_PWM
} imu_heater_fault_t;

/* 读结果时注意单位：温度 40000=40 摄氏度，加热输出 800=80%，时间 1000=1 秒。 */
typedef struct
{
    uint32_t result;
    uint32_t state;
    uint32_t fault;
    int32_t temperature_milli_c;
    int32_t target_milli_c;
    int32_t error_milli_c;
    uint32_t duty_per_mille;
    uint32_t control_count;
    uint32_t stable_time_ms;
    uint32_t sensor_wait_time_ms;
    int32_t initial_temperature_milli_c;
    int32_t minimum_temperature_milli_c;
    int32_t maximum_temperature_milli_c;
} imu_heater_summary_t;

void imu_heater_init(uint32_t current_time_ms);
void imu_heater_process(uint32_t current_time_ms);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool imu_heater_get_summary(imu_heater_summary_t *summary);

#endif
