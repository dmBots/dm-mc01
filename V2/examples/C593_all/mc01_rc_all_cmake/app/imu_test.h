/*
 * 读取板载 BMI088 惯性传感器，得到加速度（运动变化）、角速度（转动快慢）和温度。
 * init 做通信与传感器设置，主循环反复调用 process(HAL_GetTick()) 更新读数。
 * 刚开始先读温度；等 imu_heater 确认温度稳定后，再开始统计运动数据。
 * 因此预热时运动采样数量可能不增加，这是等待温度稳定，不是所有传感器读取都停止了。
 */
#ifndef IMU_TEST_H
#define IMU_TEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    IMU_TEST_RESULT_IDLE = 0U,
    IMU_TEST_RESULT_RUNNING,
    IMU_TEST_RESULT_PASSED,
    IMU_TEST_RESULT_FAILED
} imu_test_result_t;

typedef enum
{
    IMU_TEST_STEP_NONE = 0U,
    IMU_TEST_STEP_SPI_INIT,
    IMU_TEST_STEP_ACCEL_INIT,
    IMU_TEST_STEP_GYRO_INIT,
    IMU_TEST_STEP_ACCEL_RESET,
    IMU_TEST_STEP_GYRO_RESET,
    IMU_TEST_STEP_CONFIG_LOAD,
    IMU_TEST_STEP_ACCEL_POWER,
    IMU_TEST_STEP_ACCEL_CONFIG,
    IMU_TEST_STEP_GYRO_POWER,
    IMU_TEST_STEP_GYRO_CONFIG,
    IMU_TEST_STEP_ACCEL_CONFIG_READBACK,
    IMU_TEST_STEP_GYRO_CONFIG_READBACK,
    IMU_TEST_STEP_CONFIG_VERIFY,
    IMU_TEST_STEP_ACCEL_STATUS_READ,
    IMU_TEST_STEP_ACCEL_READ,
    IMU_TEST_STEP_GYRO_READ,
    IMU_TEST_STEP_TEMPERATURE_READ,
    IMU_TEST_STEP_DATA_VERIFY
} imu_test_step_t;

typedef struct
{
    uint32_t result;
    uint32_t failure_step;
    int32_t last_api_result;
    uint32_t accel_chip_id;
    uint32_t gyro_chip_id;
    uint32_t accel_odr;
    uint32_t accel_range;
    uint32_t gyro_odr;
    uint32_t gyro_range;
    uint32_t sample_count;
    uint32_t read_error_count;
    uint32_t missed_sample_count;
    uint32_t min_interval_ms;
    uint32_t max_interval_ms;
    /* raw 是芯片直接给出的数字；换算后 1000 mg=1 g，1000 mdps=1 度/秒，1000 mC=1 摄氏度。
     * g 在这里表示重力加速度单位，不是质量单位“克”；mg 也不是“毫克”。
     */
    int32_t accel_raw_x;
    int32_t accel_raw_y;
    int32_t accel_raw_z;
    int32_t gyro_raw_x;
    int32_t gyro_raw_y;
    int32_t gyro_raw_z;
    int32_t accel_mg_x;
    int32_t accel_mg_y;
    int32_t accel_mg_z;
    int32_t gyro_mdps_x;
    int32_t gyro_mdps_y;
    int32_t gyro_mdps_z;
    int32_t temperature_milli_c;
    uint32_t accel_power_conf;
    uint32_t accel_power_ctrl;
    uint32_t accel_internal_status;
    uint32_t accel_status;
    uint32_t accel_data_ready_count;
    uint32_t accel_nonzero_sample_count;
    uint32_t temperature_sample_count;
    uint32_t motion_sampling_enabled;
} imu_test_summary_t;

void imu_test_init(uint32_t current_time_ms);
void imu_test_process(uint32_t current_time_ms);
/* 读取最近温度；例如返回数值 40000 表示 40 摄氏度。返回 false 表示当前温度不可用。 */
bool imu_test_get_temperature_milli_c(int32_t *temperature_milli_c);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool imu_test_get_summary(imu_test_summary_t *summary);
/* 允许开始读取并统计运动数据，由温控模块自动调用；重复调用不会把测试从头再做。 */
void imu_test_start_sampling(void);

#endif
