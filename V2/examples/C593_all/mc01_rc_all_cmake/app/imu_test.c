#include "imu_test.h"

#include <stddef.h>
#include <string.h>

#include "bmi08x.h"
#include "board_config.h"
#include "bsp_spi2.h"

#define IMU_TEST_ACCEL_RANGE_MG     (24000)
#define IMU_TEST_GYRO_RANGE_MDPS    (2000000)

static struct bmi08_dev imu_device;
static bsp_spi2_chip_select_t accel_chip_select;
static bsp_spi2_chip_select_t gyro_chip_select;
static volatile imu_test_summary_t imu_test_status;
static uint32_t last_sample_time_ms;

static bool imu_test_check_result(int8_t api_result, imu_test_step_t step)
{
    imu_test_status.last_api_result = api_result;
    if (api_result == BMI08_OK)
    {
        return true;
    }

    imu_test_status.failure_step = step;
    imu_test_status.result = IMU_TEST_RESULT_FAILED;
    return false;
}

static BMI08_INTF_RET_TYPE imu_test_spi_read(uint8_t reg_addr,
                                             uint8_t *reg_data,
                                             uint32_t len,
                                             void *intf_ptr)
{
    uint8_t tx_data[BOARD_BMI088_SPI_READ_WRITE_LEN + 1U];
    uint8_t rx_data[BOARD_BMI088_SPI_READ_WRITE_LEN + 1U];
    bsp_spi2_chip_select_t chip_select;

    if ((reg_data == NULL) || (intf_ptr == NULL) ||
        (len > BOARD_BMI088_SPI_READ_WRITE_LEN))
    {
        return -1;
    }

    chip_select = *(bsp_spi2_chip_select_t *)intf_ptr;
    tx_data[0] = reg_addr;
    memset(&tx_data[1], 0xFF, len);
    if (!bsp_spi2_transfer(chip_select, tx_data, rx_data, len + 1U))
    {
        return -1;
    }
    memcpy(reg_data, &rx_data[1], len);
    return BMI08_INTF_RET_SUCCESS;
}

static BMI08_INTF_RET_TYPE imu_test_spi_write(uint8_t reg_addr,
                                              const uint8_t *reg_data,
                                              uint32_t len,
                                              void *intf_ptr)
{
    uint8_t tx_data[BOARD_BMI088_SPI_READ_WRITE_LEN + 1U];
    uint8_t rx_data[BOARD_BMI088_SPI_READ_WRITE_LEN + 1U];
    bsp_spi2_chip_select_t chip_select;

    if ((reg_data == NULL) || (intf_ptr == NULL) ||
        (len > BOARD_BMI088_SPI_READ_WRITE_LEN))
    {
        return -1;
    }

    chip_select = *(bsp_spi2_chip_select_t *)intf_ptr;
    tx_data[0] = reg_addr;
    memcpy(&tx_data[1], reg_data, len);
    if (!bsp_spi2_transfer(chip_select, tx_data, rx_data, len + 1U))
    {
        return -1;
    }
    return BMI08_INTF_RET_SUCCESS;
}

static void imu_test_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    bsp_spi2_delay_us(period);
}

static int32_t imu_test_scale(int16_t raw_value, int32_t full_scale)
{
    return (int32_t)(((int64_t)raw_value * full_scale) / 32768);
}

void imu_test_init(uint32_t current_time_ms)
{
    int8_t api_result;
    uint8_t register_value;

    imu_test_status = (imu_test_summary_t){0};
    imu_device = (struct bmi08_dev){0};
    imu_test_status.result = IMU_TEST_RESULT_RUNNING;
    imu_test_status.min_interval_ms = UINT32_MAX;
    last_sample_time_ms = current_time_ms;

    accel_chip_select = (bsp_spi2_chip_select_t)BOARD_BMI088_ACCEL_CHIP_SELECT;
    gyro_chip_select = (bsp_spi2_chip_select_t)BOARD_BMI088_GYRO_CHIP_SELECT;
    if (!bsp_spi2_init())
    {
        imu_test_status.failure_step = IMU_TEST_STEP_SPI_INIT;
        imu_test_status.result = IMU_TEST_RESULT_FAILED;
        return;
    }
    bsp_spi2_delay_us(BOARD_BMI088_STARTUP_DELAY_MS * 1000U);

    imu_device.intf = BMI08_SPI_INTF;
    imu_device.variant = BMI088_VARIANT;
    imu_device.read = imu_test_spi_read;
    imu_device.write = imu_test_spi_write;
    imu_device.delay_us = imu_test_delay_us;
    imu_device.intf_ptr_accel = &accel_chip_select;
    imu_device.intf_ptr_gyro = &gyro_chip_select;
    imu_device.read_write_len = BOARD_BMI088_SPI_READ_WRITE_LEN;

    api_result = bmi08xa_init(&imu_device);
    imu_test_status.accel_chip_id = imu_device.accel_chip_id;
    if (!imu_test_check_result(api_result, IMU_TEST_STEP_ACCEL_INIT))
    {
        return;
    }

    api_result = bmi08g_init(&imu_device);
    imu_test_status.gyro_chip_id = imu_device.gyro_chip_id;
    if (!imu_test_check_result(api_result, IMU_TEST_STEP_GYRO_INIT))
    {
        return;
    }

    if (!imu_test_check_result(bmi08a_soft_reset(&imu_device),
                               IMU_TEST_STEP_ACCEL_RESET) ||
        !imu_test_check_result(bmi08xa_init(&imu_device),
                               IMU_TEST_STEP_ACCEL_INIT) ||
        !imu_test_check_result(bmi08g_soft_reset(&imu_device),
                               IMU_TEST_STEP_GYRO_RESET) ||
        !imu_test_check_result(bmi08g_init(&imu_device),
                               IMU_TEST_STEP_GYRO_INIT))
    {
        return;
    }
    imu_test_status.accel_chip_id = imu_device.accel_chip_id;
    imu_test_status.gyro_chip_id = imu_device.gyro_chip_id;

    if (!imu_test_check_result(bmi08a_load_config_file(&imu_device),
                               IMU_TEST_STEP_CONFIG_LOAD))
    {
        return;
    }

    imu_device.accel_cfg.odr = BMI08_ACCEL_ODR_1600_HZ;
    imu_device.accel_cfg.range = BMI088_ACCEL_RANGE_24G;
    imu_device.accel_cfg.power = BMI08_ACCEL_PM_ACTIVE;
    imu_device.accel_cfg.bw = BMI08_ACCEL_BW_NORMAL;
    if (!imu_test_check_result(bmi08a_set_power_mode(&imu_device),
                               IMU_TEST_STEP_ACCEL_POWER) ||
        !imu_test_check_result(bmi08xa_set_meas_conf(&imu_device),
                               IMU_TEST_STEP_ACCEL_CONFIG))
    {
        return;
    }

    imu_device.gyro_cfg.odr = BMI08_GYRO_BW_230_ODR_2000_HZ;
    imu_device.gyro_cfg.range = BMI08_GYRO_RANGE_2000_DPS;
    imu_device.gyro_cfg.bw = BMI08_GYRO_BW_230_ODR_2000_HZ;
    imu_device.gyro_cfg.power = BMI08_GYRO_PM_NORMAL;
    if (!imu_test_check_result(bmi08g_set_power_mode(&imu_device),
                               IMU_TEST_STEP_GYRO_POWER) ||
        !imu_test_check_result(bmi08g_set_meas_conf(&imu_device),
                               IMU_TEST_STEP_GYRO_CONFIG) ||
        !imu_test_check_result(bmi08a_get_meas_conf(&imu_device),
                               IMU_TEST_STEP_ACCEL_CONFIG_READBACK) ||
        !imu_test_check_result(bmi08g_get_meas_conf(&imu_device),
                               IMU_TEST_STEP_GYRO_CONFIG_READBACK))
    {
        return;
    }

    imu_test_status.accel_odr = imu_device.accel_cfg.odr;
    imu_test_status.accel_range = imu_device.accel_cfg.range;
    imu_test_status.gyro_odr = imu_device.gyro_cfg.odr;
    imu_test_status.gyro_range = imu_device.gyro_cfg.range;
    if (!imu_test_check_result(bmi08a_get_set_regs(BMI08_REG_ACCEL_PWR_CONF,
                                                   &register_value, 1U, &imu_device, GET_FUNC),
                               IMU_TEST_STEP_ACCEL_CONFIG_READBACK))
    {
        return;
    }
    imu_test_status.accel_power_conf = register_value;
    if (!imu_test_check_result(bmi08a_get_set_regs(BMI08_REG_ACCEL_PWR_CTRL,
                                                   &register_value, 1U, &imu_device, GET_FUNC),
                               IMU_TEST_STEP_ACCEL_CONFIG_READBACK))
    {
        return;
    }
    imu_test_status.accel_power_ctrl = register_value;
    if (!imu_test_check_result(bmi08a_get_set_regs(BMI08_REG_ACCEL_INTERNAL_STAT,
                                                   &register_value, 1U, &imu_device, GET_FUNC),
                               IMU_TEST_STEP_ACCEL_CONFIG_READBACK))
    {
        return;
    }
    imu_test_status.accel_internal_status = register_value;
    if ((imu_device.accel_cfg.odr != BMI08_ACCEL_ODR_1600_HZ) ||
        (imu_device.accel_cfg.range != BMI088_ACCEL_RANGE_24G) ||
        (imu_device.gyro_cfg.odr != BMI08_GYRO_BW_230_ODR_2000_HZ) ||
        (imu_device.gyro_cfg.range != BMI08_GYRO_RANGE_2000_DPS))
    {
        imu_test_status.failure_step = IMU_TEST_STEP_CONFIG_VERIFY;
        imu_test_status.result = IMU_TEST_RESULT_FAILED;
        return;
    }

    last_sample_time_ms = HAL_GetTick();
}

void imu_test_process(uint32_t current_time_ms)
{
    struct bmi08_sensor_data accel_data;
    struct bmi08_sensor_data gyro_data;
    int32_t temperature;
    uint32_t interval_ms;
    int8_t api_result;
    uint8_t accel_status;

    if ((imu_test_status.result == IMU_TEST_RESULT_IDLE) ||
        (imu_test_status.result == IMU_TEST_RESULT_FAILED))
    {
        return;
    }

    interval_ms = current_time_ms - last_sample_time_ms;
    if (interval_ms < BOARD_BMI088_POLL_INTERVAL_MS)
    {
        return;
    }
    last_sample_time_ms = current_time_ms;

    if (interval_ms < imu_test_status.min_interval_ms)
    {
        imu_test_status.min_interval_ms = interval_ms;
    }
    if (interval_ms > imu_test_status.max_interval_ms)
    {
        imu_test_status.max_interval_ms = interval_ms;
    }
    if (interval_ms > BOARD_BMI088_POLL_INTERVAL_MS)
    {
        imu_test_status.missed_sample_count +=
            (interval_ms / BOARD_BMI088_POLL_INTERVAL_MS) - 1U;
    }

    api_result = bmi08a_get_sensor_temperature(&imu_device, &temperature);
    if (!imu_test_check_result(api_result, IMU_TEST_STEP_TEMPERATURE_READ))
    {
        imu_test_status.read_error_count++;
        return;
    }
    imu_test_status.temperature_milli_c = temperature;
    imu_test_status.temperature_sample_count++;
    /* 预热时先持续读取温度；还没允许正式采样，就先不读取和统计加速度、角速度。 */
    if (imu_test_status.motion_sampling_enabled == 0U)
    {
        return;
    }

    api_result = bmi08a_get_status(&accel_status, &imu_device);
    imu_test_status.accel_status = accel_status;
    if (!imu_test_check_result(api_result, IMU_TEST_STEP_ACCEL_STATUS_READ))
    {
        imu_test_status.read_error_count++;
        return;
    }
    if (accel_status != 0U)
    {
        imu_test_status.accel_data_ready_count++;
    }

    api_result = bmi08a_get_data(&accel_data, &imu_device);
    if (!imu_test_check_result(api_result, IMU_TEST_STEP_ACCEL_READ))
    {
        imu_test_status.read_error_count++;
        return;
    }
    api_result = bmi08g_get_data(&gyro_data, &imu_device);
    if (!imu_test_check_result(api_result, IMU_TEST_STEP_GYRO_READ))
    {
        imu_test_status.read_error_count++;
        return;
    }

    imu_test_status.accel_raw_x = accel_data.x;
    imu_test_status.accel_raw_y = accel_data.y;
    imu_test_status.accel_raw_z = accel_data.z;
    imu_test_status.gyro_raw_x = gyro_data.x;
    imu_test_status.gyro_raw_y = gyro_data.y;
    imu_test_status.gyro_raw_z = gyro_data.z;
    imu_test_status.accel_mg_x = imu_test_scale(accel_data.x, IMU_TEST_ACCEL_RANGE_MG);
    imu_test_status.accel_mg_y = imu_test_scale(accel_data.y, IMU_TEST_ACCEL_RANGE_MG);
    imu_test_status.accel_mg_z = imu_test_scale(accel_data.z, IMU_TEST_ACCEL_RANGE_MG);
    imu_test_status.gyro_mdps_x = imu_test_scale(gyro_data.x, IMU_TEST_GYRO_RANGE_MDPS);
    imu_test_status.gyro_mdps_y = imu_test_scale(gyro_data.y, IMU_TEST_GYRO_RANGE_MDPS);
    imu_test_status.gyro_mdps_z = imu_test_scale(gyro_data.z, IMU_TEST_GYRO_RANGE_MDPS);
    imu_test_status.sample_count++;
    if ((accel_data.x != 0) || (accel_data.y != 0) || (accel_data.z != 0))
    {
        imu_test_status.accel_nonzero_sample_count++;
    }
    if (imu_test_status.sample_count >= BOARD_BMI088_TEST_SAMPLE_COUNT)
    {
        if ((imu_test_status.accel_data_ready_count == 0U) ||
            (imu_test_status.accel_nonzero_sample_count == 0U) ||
            (temperature < -40000) || (temperature > 85000))
        {
            imu_test_status.failure_step = IMU_TEST_STEP_DATA_VERIFY;
            imu_test_status.result = IMU_TEST_RESULT_FAILED;
        }
        else
        {
            imu_test_status.result = IMU_TEST_RESULT_PASSED;
        }
    }
}

bool imu_test_get_temperature_milli_c(int32_t *temperature_milli_c)
{
    if ((temperature_milli_c == NULL) ||
        (imu_test_status.temperature_sample_count == 0U) ||
        (imu_test_status.result == IMU_TEST_RESULT_FAILED))
    {
        return false;
    }

    *temperature_milli_c = imu_test_status.temperature_milli_c;
    return true;
}

bool imu_test_get_summary(imu_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    *summary = imu_test_status;
    return true;
}

void imu_test_start_sampling(void)
{
    if ((imu_test_status.result != IMU_TEST_RESULT_FAILED) &&
        (imu_test_status.temperature_sample_count != 0U))
    {
        imu_test_status.motion_sampling_enabled = 1U;
    }
}
