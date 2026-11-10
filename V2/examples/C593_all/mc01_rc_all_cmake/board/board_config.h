/*
 * 想调整闪灯速度、目标温度或测试条件时，先在本文件查找对应参数。
 * #define 为参数起名字，改数值后要重新编译并烧录，板子才会使用新设置。
 * 名称以 _MS 结尾表示毫秒，1000 就是 1 秒；以 _MV 结尾表示毫伏，1000 就是 1 伏。
 * _MILLI_C 表示温度，40000 就是 40 摄氏度；_PER_MILLE 表示千分比，800 就是 80%。
 * GPIO 表示芯片引脚；如需改接线或定时器，不能只改这里，还要检查 CubeMX 生成的硬件配置。
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "stm32_hal.h"

/* LED 接在哪个引脚、什么电平会亮，以及按键按下时是什么电平。
 * 闪烁间隔越短闪得越快；消抖时间表示按键变化后等多久再确认一次按下。
 */
#define BOARD_USER_LED_GPIO_PORT             (GPIOC_BASE)
#define BOARD_USER_LED_GPIO_PIN              (HAL_GPIO_PIN_0)
#define BOARD_USER_LED_ACTIVE_STATE           HAL_GPIO_PIN_RESET
#define BOARD_USER_LED_INACTIVE_STATE         HAL_GPIO_PIN_SET
#define BOARD_USER_LED_SLOW_INTERVAL_MS       (1000U)
#define BOARD_USER_LED_NORMAL_INTERVAL_MS     (500U)
#define BOARD_USER_LED_FAST_INTERVAL_MS       (100U)

#define BOARD_USER_KEY_GPIO_PORT              (GPIOA_BASE)
#define BOARD_USER_KEY_GPIO_PIN               (HAL_GPIO_PIN_15)
#define BOARD_USER_KEY_ACTIVE_STATE           HAL_GPIO_PIN_RESET
#define BOARD_USER_KEY_DEBOUNCE_MS             (30U)

/* 外设的 5 V 供电和惯性传感器的加热供电是两个独立开关。 */
#define BOARD_OUTER_POWER_5V_GPIO_PORT       (GPIOC_BASE)
#define BOARD_OUTER_POWER_5V_GPIO_PIN        (HAL_GPIO_PIN_14)

#define BOARD_HEAT_POWER_5V_GPIO_PORT        (GPIOC_BASE)
#define BOARD_HEAT_POWER_5V_GPIO_PIN         (HAL_GPIO_PIN_15)

/* 默认加热到 40 摄氏度，连续 10 秒保持在目标上下 0.5 摄氏度内，才算温度稳定。
 * 最大加热输出为 80%，不是一上电就始终全功率加热；KP/KI 用于计算自动调温的输出。
 */
#define BOARD_IMU_HEATER_TARGET_MILLI_C      (40000)
#define BOARD_IMU_HEATER_OVER_TEMP_MILLI_C   (60000)
#define BOARD_IMU_HEATER_CONTROL_INTERVAL_MS (20U)
#define BOARD_IMU_HEATER_SENSOR_TIMEOUT_MS   (500U)
#define BOARD_IMU_HEATER_STABLE_BAND_MILLI_C (500)
#define BOARD_IMU_HEATER_STABLE_TIME_MS      (10000U)
#define BOARD_IMU_HEATER_MAX_DUTY_PER_MILLE  (800U)
#define BOARD_IMU_HEATER_KP_PER_MILLE_PER_C  (20)
#define BOARD_IMU_HEATER_KI_PER_MILLE_PER_C_S (5)

/* 蜂鸣器用周期信号发声；频率决定音调，DUTY_PERCENT=50 表示一半时间输出有效信号。
 * TIMER_CLOCK_HZ 是芯片给蜂鸣器定时器的时钟频率，不是蜂鸣器发声的频率。
 */
#define BOARD_BEEP_TIMER_CLOCK_HZ              (144000000UL)
#define BOARD_BEEP_DEFAULT_FREQUENCY_HZ        (4000U)
#define BOARD_BEEP_DUTY_PERCENT                (50U)

/* 预期脉冲数=每秒脉冲数x测试秒数；默认 1000 Hz 测 500 ms，预期约 500 次。
 * 这里用于计算合格范围，实际发脉冲速度还由生成的 TIM8 硬件配置决定，两处必须一致。
 */
#define BOARD_TIM8_PWM_FREQUENCY_HZ             (1000U)
#define BOARD_TIM8_LOOPBACK_TEST_TIME_MS         (500U)
#define BOARD_TIM8_LOOPBACK_TOLERANCE_PERCENT   (10U)

/* BMI088 内的加速度计和陀螺仪共用 SPI2 通信线，用两个片选引脚选择要访问哪一个。 */
#define BOARD_SPI2_CS0                         (0U)
#define BOARD_SPI2_CS1                         (1U)
#define BOARD_SPI2_CS0_GPIO_PORT               (GPIOB_BASE)
#define BOARD_SPI2_CS0_GPIO_PIN                (HAL_GPIO_PIN_1)
#define BOARD_SPI2_CS1_GPIO_PORT               (GPIOC_BASE)
#define BOARD_SPI2_CS1_GPIO_PIN                (HAL_GPIO_PIN_5)
#define BOARD_SPI2_CS_ACTIVE_STATE             HAL_GPIO_PIN_RESET
#define BOARD_SPI2_CS_INACTIVE_STATE           HAL_GPIO_PIN_SET
#define BOARD_SPI2_TRANSFER_TIMEOUT_MS         (2U)

/* 屏幕的接线、像素宽高和等待时间；X/Y_OFFSET 用于对齐屏幕可见区域，通常不必改动。 */
#define BOARD_LCD_CS_GPIO_PORT                 (GPIOA_BASE)
#define BOARD_LCD_CS_GPIO_PIN                  (HAL_GPIO_PIN_4)
#define BOARD_LCD_BACKLIGHT_GPIO_PORT          (GPIOB_BASE)
#define BOARD_LCD_BACKLIGHT_GPIO_PIN           (HAL_GPIO_PIN_3)
#define BOARD_LCD_RESET_GPIO_PORT              (GPIOB_BASE)
#define BOARD_LCD_RESET_GPIO_PIN               (HAL_GPIO_PIN_4)
#define BOARD_LCD_DC_GPIO_PORT                 (GPIOC_BASE)
#define BOARD_LCD_DC_GPIO_PIN                  (HAL_GPIO_PIN_1)
#define BOARD_LCD_WIDTH                        (280U)
#define BOARD_LCD_HEIGHT                       (240U)
#define BOARD_LCD_X_OFFSET                     (20U)
#define BOARD_LCD_Y_OFFSET                     (0U)
#define BOARD_LCD_DMA_BUFFER_SIZE              (512U)
#define BOARD_LCD_RESET_PULSE_MS               (100U)
#define BOARD_LCD_RESET_RECOVERY_MS            (100U)
#define BOARD_LCD_BACKLIGHT_STABILIZE_MS       (100U)
#define BOARD_LCD_SLEEP_OUT_MS                 (120U)
#define BOARD_LCD_TRANSFER_TIMEOUT_MS          (100U)
#define BOARD_LCD_TEST_TIMEOUT_MS              (5000U)

/* 惯性传感器的读取间隔和测试次数；运动数据要等加热温度稳定后才开始计数。 */
#define BOARD_BMI088_ACCEL_CHIP_SELECT         BOARD_SPI2_CS0
#define BOARD_BMI088_GYRO_CHIP_SELECT          BOARD_SPI2_CS1
#define BOARD_BMI088_SPI_READ_WRITE_LEN        (32U)
#define BOARD_BMI088_STARTUP_DELAY_MS           (10U)
#define BOARD_BMI088_POLL_INTERVAL_MS          (1U)
#define BOARD_BMI088_TEST_SAMPLE_COUNT         (1000U)

#define BOARD_CAN_PRESET_CLASSIC_1M           (0U)
#define BOARD_CAN_PRESET_CANFD_5M             (1U)
#define BOARD_CAN_PRESET_CANFD_8M             (2U)

/* 选择 CAN 通信速度；通信双方设置不一致时，即使接线正确也可能无法正常通信。 */
#define BOARD_CAN_PRESET                      BOARD_CAN_PRESET_CANFD_8M
#define BOARD_CAN_TRANSCEIVER_STARTUP_MS      (100U)
#define BOARD_CAN_SELF_TEST_TIMEOUT_MS        (100U)

#define BOARD_USB_TEST_ECHO                    (0U)
#define BOARD_USB_TEST_VOFA_JUSTFLOAT          (1U)

/* 只有运行独立 usb_test 演示时，这个设置才选择“原样回传”或“发送绘图数据”。
 * 当前 main 使用测试报告功能，修改此项不会把报告改成波形输出。
 */
#define BOARD_USB_TEST_MODE                    BOARD_USB_TEST_VOFA_JUSTFLOAT
#define BOARD_USB_VOFA_SAMPLE_INTERVAL_MS      (10U)
#define BOARD_USB_VOFA_SINE_FREQUENCY_HZ       (1.0F)
#define BOARD_USB_VOFA_SINE_AMPLITUDE          (1.0F)
/* 整份报告最多等多久；默认 180000 毫秒就是 3 分钟。
 * 超时会给出最终失败结论，但不代表所有外设处理都已停止，也不是加热电源开关。
 */
#define BOARD_FACTORY_TEST_TIMEOUT_MS           (180000U)

/* RS485 通信速度和缓存大小；缓存是程序暂存收发数据的数组。
 * RX_TIMEOUT_BITS 表示空闲多少个位的时间后取出短包，不是等待多少毫秒。
 */
#define BOARD_RS485_BAUD_RATE                  (2000000U)
#define BOARD_RS485_DMA_BUFFER_SIZE            (128U)
#define BOARD_RS485_RX_QUEUE_SIZE              (512U)
#define BOARD_RS485_RX_TIMEOUT_BITS            (150U)
#define BOARD_RS485_ECHO_SHORT_SIZE            (17U)
#define BOARD_RS485_TRANSCEIVER_STARTUP_MS     (10U)
#define BOARD_RS485_ECHO_TIMEOUT_MS            (100U)

/* 串口回环测试：用较短、半个缓存长度、整个缓存长度三种数据检查接收是否正常。 */
#define BOARD_UART_LOOPBACK_BAUD_RATE           (115200U)
#define BOARD_UART_DMA_BUFFER_SIZE              (128U)
#define BOARD_UART_RX_QUEUE_SIZE                (512U)
#define BOARD_UART_RX_TIMEOUT_BITS              (150U)
#define BOARD_UART_LOOPBACK_SHORT_SIZE          (17U)
#define BOARD_UART_LOOPBACK_TIMEOUT_MS          (100U)

/* 仅供独立 SBUS 接收示例使用；当前 main 没有调用它，UART7 格式由生成代码设置。 */
#define BOARD_SBUS_BAUD_RATE                     (100000U)
#define BOARD_SBUS_FRAME_SIZE                    (25U)
#define BOARD_SBUS_DMA_BUFFER_SIZE               (50U)
#define BOARD_SBUS_RX_QUEUE_SIZE                 (256U)
#define BOARD_SBUS_TEST_VALID_FRAME_COUNT        (20U)
#define BOARD_SBUS_TEST_TIMEOUT_MS               (1000U)
#define BOARD_SBUS_PROCESS_BUDGET_BYTES          (100U)

/* 电池电压先经过板上电阻分压，再由 ADC 测量，最后换算回电池端电压。
 * MIN/MAX_MV 是自动测试的合格范围，不等于硬件允许的供电范围，供电应遵循开发板说明。
 * MAX_SPREAD_RAW 是最大读数与最小读数之差的上限，不是允许波动多少毫伏。
 */
#define BOARD_ADC_REFERENCE_MV                    (3300U)
#define BOARD_ADC_12BIT_FULL_SCALE                (4095U)
#define BOARD_VBAT_DIVIDER_HIGH_OHM               (100000U)
#define BOARD_VBAT_DIVIDER_LOW_OHM                (10000U)
#define BOARD_VBAT_SAMPLE_INTERVAL_MS             (10U)
#define BOARD_VBAT_SAMPLE_TIMEOUT_MS              (10U)
#define BOARD_VBAT_TEST_SAMPLE_COUNT              (100U)
#define BOARD_VBAT_TEST_MIN_MV                    (18000U)
#define BOARD_VBAT_TEST_MAX_MV                    (30000U)
#define BOARD_VBAT_TEST_MAX_SPREAD_RAW            (100U)
/* 模拟按键多久读一次、收集多少次、屏幕多久更新一次；与切换 LED 的用户按键不同。 */
#define BOARD_KEY_ADC_SAMPLE_INTERVAL_MS           (20U)
#define BOARD_KEY_ADC_SAMPLE_TIMEOUT_MS            (10U)
#define BOARD_KEY_ADC_TEST_SAMPLE_COUNT            (100U)
#define BOARD_KEY_ADC_DISPLAY_REFRESH_MS           (100U)

#if (BOARD_USER_LED_SLOW_INTERVAL_MS == 0U) || \
    (BOARD_USER_LED_NORMAL_INTERVAL_MS == 0U) || \
    (BOARD_USER_LED_FAST_INTERVAL_MS == 0U) || \
    (BOARD_USER_KEY_DEBOUNCE_MS == 0U)
#error "Invalid user key or LED mode configuration"
#endif

#if (BOARD_CAN_PRESET != BOARD_CAN_PRESET_CLASSIC_1M) && \
    (BOARD_CAN_PRESET != BOARD_CAN_PRESET_CANFD_5M) && \
    (BOARD_CAN_PRESET != BOARD_CAN_PRESET_CANFD_8M)
#error "Unsupported BOARD_CAN_PRESET"
#endif

#if (BOARD_USB_TEST_MODE != BOARD_USB_TEST_ECHO) && \
    (BOARD_USB_TEST_MODE != BOARD_USB_TEST_VOFA_JUSTFLOAT)
#error "Unsupported BOARD_USB_TEST_MODE"
#endif

#if BOARD_FACTORY_TEST_TIMEOUT_MS == 0U
#error "Factory test timeout must be non-zero"
#endif

#if (BOARD_RS485_DMA_BUFFER_SIZE == 0U) || ((BOARD_RS485_DMA_BUFFER_SIZE % 2U) != 0U)
#error "BOARD_RS485_DMA_BUFFER_SIZE must be a non-zero even number"
#endif

#if (BOARD_RS485_RX_QUEUE_SIZE <= BOARD_RS485_DMA_BUFFER_SIZE)
#error "BOARD_RS485_RX_QUEUE_SIZE must be larger than BOARD_RS485_DMA_BUFFER_SIZE"
#endif

#if (BOARD_RS485_ECHO_SHORT_SIZE == 0U) || \
    (BOARD_RS485_ECHO_SHORT_SIZE >= (BOARD_RS485_DMA_BUFFER_SIZE / 2U))
#error "BOARD_RS485_ECHO_SHORT_SIZE must exercise the receiver-timeout path"
#endif

#if (BOARD_UART_DMA_BUFFER_SIZE == 0U) || ((BOARD_UART_DMA_BUFFER_SIZE % 2U) != 0U)
#error "BOARD_UART_DMA_BUFFER_SIZE must be a non-zero even number"
#endif

#if (BOARD_UART_RX_QUEUE_SIZE <= BOARD_UART_DMA_BUFFER_SIZE)
#error "BOARD_UART_RX_QUEUE_SIZE must be larger than BOARD_UART_DMA_BUFFER_SIZE"
#endif

#if (BOARD_UART_LOOPBACK_SHORT_SIZE == 0U) || \
    (BOARD_UART_LOOPBACK_SHORT_SIZE >= (BOARD_UART_DMA_BUFFER_SIZE / 2U))
#error "BOARD_UART_LOOPBACK_SHORT_SIZE must exercise the receiver-timeout path"
#endif

#if (BOARD_SBUS_FRAME_SIZE != 25U) || \
    (BOARD_SBUS_DMA_BUFFER_SIZE != (2U * BOARD_SBUS_FRAME_SIZE))
#error "SBUS DMA buffer must contain exactly two 25-byte frames"
#endif

#if (BOARD_SBUS_RX_QUEUE_SIZE <= BOARD_SBUS_DMA_BUFFER_SIZE) || \
    (BOARD_SBUS_TEST_VALID_FRAME_COUNT == 0U) || \
    (BOARD_SBUS_PROCESS_BUDGET_BYTES == 0U)
#error "Invalid SBUS queue or test configuration"
#endif

#if (BOARD_ADC_REFERENCE_MV == 0U) || (BOARD_ADC_12BIT_FULL_SCALE == 0U) || \
    (BOARD_VBAT_DIVIDER_LOW_OHM == 0U) || (BOARD_VBAT_SAMPLE_INTERVAL_MS == 0U) || \
    (BOARD_VBAT_SAMPLE_TIMEOUT_MS == 0U) || (BOARD_VBAT_TEST_SAMPLE_COUNT == 0U)
#error "Invalid VBAT ADC configuration"
#endif

#if (BOARD_VBAT_TEST_MIN_MV >= BOARD_VBAT_TEST_MAX_MV)
#error "Invalid VBAT test voltage range"
#endif

#if (BOARD_KEY_ADC_SAMPLE_INTERVAL_MS == 0U) || \
    (BOARD_KEY_ADC_SAMPLE_TIMEOUT_MS == 0U) || \
    (BOARD_KEY_ADC_TEST_SAMPLE_COUNT == 0U) || \
    (BOARD_KEY_ADC_DISPLAY_REFRESH_MS == 0U)
#error "Invalid key ADC configuration"
#endif

#if (BOARD_BMI088_ACCEL_CHIP_SELECT == BOARD_BMI088_GYRO_CHIP_SELECT)
#error "BMI088 accel and gyro must use different chip-select signals"
#endif

#if (BOARD_BMI088_POLL_INTERVAL_MS == 0U) || (BOARD_BMI088_SPI_READ_WRITE_LEN == 0U)
#error "BMI088 polling interval and SPI transfer length must be non-zero"
#endif

#if (BOARD_LCD_WIDTH == 0U) || (BOARD_LCD_HEIGHT == 0U) || \
    (BOARD_LCD_DMA_BUFFER_SIZE < 2U) || ((BOARD_LCD_DMA_BUFFER_SIZE % 2U) != 0U) || \
    (BOARD_LCD_RESET_PULSE_MS == 0U) || (BOARD_LCD_RESET_RECOVERY_MS == 0U) || \
    (BOARD_LCD_BACKLIGHT_STABILIZE_MS == 0U) || (BOARD_LCD_SLEEP_OUT_MS == 0U) || \
    (BOARD_LCD_TRANSFER_TIMEOUT_MS == 0U) || \
    (BOARD_LCD_TEST_TIMEOUT_MS == 0U)
#error "Invalid LCD configuration"
#endif

#if (BOARD_IMU_HEATER_CONTROL_INTERVAL_MS == 0U) || \
    (BOARD_IMU_HEATER_MAX_DUTY_PER_MILLE > 1000U) || \
    (BOARD_IMU_HEATER_KI_PER_MILLE_PER_C_S == 0)
#error "Invalid IMU heater control interval or duty limit"
#endif

#if (BOARD_IMU_HEATER_TARGET_MILLI_C >= BOARD_IMU_HEATER_OVER_TEMP_MILLI_C)
#error "IMU heater target must be below the over-temperature limit"
#endif

#if (BOARD_TIM8_PWM_FREQUENCY_HZ == 0U) || \
    (BOARD_TIM8_LOOPBACK_TEST_TIME_MS == 0U) || \
    (BOARD_TIM8_LOOPBACK_TOLERANCE_PERCENT >= 100U)
#error "Invalid TIM8 loopback test configuration"
#endif

#endif
