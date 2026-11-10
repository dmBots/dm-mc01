#include "imu_heater.h"

#include <limits.h>
#include <stddef.h>

#include "board_config.h"
#include "bsp_heater.h"
#include "bsp_pwr.h"
#include "imu_test.h"

/* 把之前的温差按经过的时间累加，作为 PI 中的“积分”。
 * 温度以 0.001 摄氏度保存、时间以毫秒保存，因此除以 1000000 才换成摄氏度乘秒。
 */
#define IMU_HEATER_INTEGRAL_SCALE (1000000LL)

static volatile imu_heater_summary_t imu_heater_status;
static int64_t integral_error_milli_c_ms;
static uint32_t last_control_time_ms;

static void imu_heater_fail(imu_heater_fault_t fault)
{
    (void)bsp_heater_shutdown();
    bsp_pwr_heat_5v_cmd(false);
    imu_heater_status.duty_per_mille = 0U;
    imu_heater_status.fault = fault;
    imu_heater_status.state = IMU_HEATER_STATE_FAULT;
    imu_heater_status.result = IMU_HEATER_RESULT_FAILED;
}

static uint16_t imu_heater_calculate_duty(int32_t error_milli_c,
                                          uint32_t interval_ms)
{
    int64_t proportional_duty;
    int64_t candidate_integral;
    int64_t candidate_integral_duty;
    int64_t candidate_output;
    int64_t integral_limit;
    int64_t output;

    proportional_duty = ((int64_t)error_milli_c *
                         BOARD_IMU_HEATER_KP_PER_MILLE_PER_C) / 1000LL;
    candidate_integral = integral_error_milli_c_ms +
                         ((int64_t)error_milli_c * interval_ms);
    integral_limit = ((int64_t)BOARD_IMU_HEATER_MAX_DUTY_PER_MILLE *
                      IMU_HEATER_INTEGRAL_SCALE) /
                     BOARD_IMU_HEATER_KI_PER_MILLE_PER_C_S;
    if (candidate_integral < 0LL)
    {
        candidate_integral = 0LL;
    }
    else if (candidate_integral > integral_limit)
    {
        candidate_integral = integral_limit;
    }

    candidate_integral_duty = (candidate_integral *
                               BOARD_IMU_HEATER_KI_PER_MILLE_PER_C_S) /
                              IMU_HEATER_INTEGRAL_SCALE;
    candidate_output = proportional_duty + candidate_integral_duty;
    /* 已经想输出超过上限、温度却还偏低时，先不继续累加；温度偏高后仍允许累加值减小。 */
    if (!((candidate_output > BOARD_IMU_HEATER_MAX_DUTY_PER_MILLE) &&
          (error_milli_c > 0)))
    {
        integral_error_milli_c_ms = candidate_integral;
    }

    output = proportional_duty +
             ((integral_error_milli_c_ms * BOARD_IMU_HEATER_KI_PER_MILLE_PER_C_S) /
              IMU_HEATER_INTEGRAL_SCALE);
    if (output < 0LL)
    {
        output = 0LL;
    }
    else if (output > BOARD_IMU_HEATER_MAX_DUTY_PER_MILLE)
    {
        output = BOARD_IMU_HEATER_MAX_DUTY_PER_MILLE;
    }
    return (uint16_t)output;
}

void imu_heater_init(uint32_t current_time_ms)
{
    imu_heater_status = (imu_heater_summary_t){0};
    imu_heater_status.result = IMU_HEATER_RESULT_RUNNING;
    imu_heater_status.state = IMU_HEATER_STATE_WAIT_SENSOR;
    imu_heater_status.target_milli_c = BOARD_IMU_HEATER_TARGET_MILLI_C;
    imu_heater_status.minimum_temperature_milli_c = INT32_MAX;
    imu_heater_status.maximum_temperature_milli_c = INT32_MIN;
    integral_error_milli_c_ms = 0LL;
    last_control_time_ms = current_time_ms;

    bsp_pwr_heat_5v_cmd(false);
    if (!bsp_heater_init())
    {
        imu_heater_fail(IMU_HEATER_FAULT_BSP_INIT);
        return;
    }
    bsp_pwr_heat_5v_cmd(true);
}

void imu_heater_process(uint32_t current_time_ms)
{
    uint32_t interval_ms = current_time_ms - last_control_time_ms;
    int32_t temperature_milli_c;
    int32_t absolute_error_milli_c;
    uint16_t duty_per_mille;

    if ((imu_heater_status.result == IMU_HEATER_RESULT_FAILED) ||
        (interval_ms < BOARD_IMU_HEATER_CONTROL_INTERVAL_MS))
    {
        return;
    }
    last_control_time_ms = current_time_ms;

    if (!imu_test_get_temperature_milli_c(&temperature_milli_c))
    {
        imu_heater_status.sensor_wait_time_ms += interval_ms;
        (void)bsp_heater_set_duty_per_mille(0U);
        imu_heater_status.duty_per_mille = 0U;
        if (imu_heater_status.sensor_wait_time_ms >= BOARD_IMU_HEATER_SENSOR_TIMEOUT_MS)
        {
            imu_heater_fail(IMU_HEATER_FAULT_SENSOR_TIMEOUT);
        }
        return;
    }

    imu_heater_status.sensor_wait_time_ms = 0U;
    imu_heater_status.temperature_milli_c = temperature_milli_c;
    if (imu_heater_status.control_count == 0U)
    {
        imu_heater_status.initial_temperature_milli_c = temperature_milli_c;
    }
    if (temperature_milli_c < imu_heater_status.minimum_temperature_milli_c)
    {
        imu_heater_status.minimum_temperature_milli_c = temperature_milli_c;
    }
    if (temperature_milli_c > imu_heater_status.maximum_temperature_milli_c)
    {
        imu_heater_status.maximum_temperature_milli_c = temperature_milli_c;
    }
    if (temperature_milli_c >= BOARD_IMU_HEATER_OVER_TEMP_MILLI_C)
    {
        imu_heater_fail(IMU_HEATER_FAULT_OVER_TEMPERATURE);
        return;
    }

    imu_heater_status.error_milli_c = BOARD_IMU_HEATER_TARGET_MILLI_C - temperature_milli_c;
    duty_per_mille = imu_heater_calculate_duty(imu_heater_status.error_milli_c,
                                               interval_ms);
    if (!bsp_heater_set_duty_per_mille(duty_per_mille))
    {
        imu_heater_fail(IMU_HEATER_FAULT_PWM);
        return;
    }
    imu_heater_status.duty_per_mille = duty_per_mille;
    imu_heater_status.control_count++;

    absolute_error_milli_c = imu_heater_status.error_milli_c;
    if (absolute_error_milli_c < 0)
    {
        absolute_error_milli_c = -absolute_error_milli_c;
    }
    /* 必须连续保持在允许温差内才算稳定；中途超出范围，就重新开始计算这段等待时间。 */
    if (absolute_error_milli_c <= BOARD_IMU_HEATER_STABLE_BAND_MILLI_C)
    {
        imu_heater_status.state = IMU_HEATER_STATE_REGULATING;
        imu_heater_status.stable_time_ms += interval_ms;
        if (imu_heater_status.stable_time_ms >= BOARD_IMU_HEATER_STABLE_TIME_MS)
        {
            imu_test_start_sampling();
            imu_heater_status.state = IMU_HEATER_STATE_STABLE;
            imu_heater_status.result = IMU_HEATER_RESULT_PASSED;
        }
    }
    else
    {
        imu_heater_status.stable_time_ms = 0U;
        imu_heater_status.state = (imu_heater_status.error_milli_c > 0) ?
                                  IMU_HEATER_STATE_WARMUP :
                                  IMU_HEATER_STATE_REGULATING;
    }
}

bool imu_heater_get_summary(imu_heater_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    *summary = imu_heater_status;
    return true;
}
