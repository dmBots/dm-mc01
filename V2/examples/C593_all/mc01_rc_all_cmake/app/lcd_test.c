#include "lcd_test.h"

#include <stdbool.h>
#include <stddef.h>

#include "board_config.h"
#include "bsp_lcd.h"
#include "key_test.h"
#include "lcd.h"
#include "lcd_font.h"
#include "lcd_picture.h"

#define LCD_TEST_FAILURE_INITIALIZATION  (1UL << 0U)
#define LCD_TEST_FAILURE_DRIVER          (1UL << 1U)
#define LCD_TEST_FAILURE_TIMEOUT         (1UL << 2U)
#define LCD_TEST_FAILURE_COUNTERS        (1UL << 3U)
#define LCD_TEST_FAILURE_FONT            (1UL << 4U)

/* 这些数字是画完当前演示画面应产生的统计；若改文字、图片或分块大小，也要重新核对这些值。 */
#define LCD_TEST_INITIAL_DMA_TRANSFERS   (383U)
#define LCD_TEST_INITIAL_BYTES           (163847U)
#define LCD_TEST_INITIAL_COMMANDS        (15U)
#define LCD_TEST_INITIAL_OPERATIONS      (7U)
#define LCD_TEST_INITIAL_PIXELS          (81856U)
#define LCD_TEST_RENDER_BUFFER_SIZE      (8192U)

typedef enum
{
    LCD_TEST_RESULT_IDLE = 0U,
    LCD_TEST_RESULT_RUNNING,
    LCD_TEST_RESULT_PASSED,
    LCD_TEST_RESULT_FAILED
} lcd_test_result_t;

typedef enum
{
    LCD_TEST_PHASE_IDLE = 0U,
    LCD_TEST_PHASE_WAIT_INITIALIZATION,
    LCD_TEST_PHASE_CLEAR,
    LCD_TEST_PHASE_WAIT_CLEAR,
    LCD_TEST_PHASE_DRAW_DMBOT,
    LCD_TEST_PHASE_WAIT_DMBOT,
    LCD_TEST_PHASE_DRAW_CHINESE,
    LCD_TEST_PHASE_WAIT_CHINESE,
    LCD_TEST_PHASE_DRAW_LEFT_LINE,
    LCD_TEST_PHASE_WAIT_LEFT_LINE,
    LCD_TEST_PHASE_DRAW_RIGHT_LINE,
    LCD_TEST_PHASE_WAIT_RIGHT_LINE,
    LCD_TEST_PHASE_DRAW_IMAGE,
    LCD_TEST_PHASE_WAIT_IMAGE,
    LCD_TEST_PHASE_DRAW_KEY_VALUE,
    LCD_TEST_PHASE_WAIT_KEY_VALUE,
    LCD_TEST_PHASE_ACTIVE,
    LCD_TEST_PHASE_FAILED
} lcd_test_phase_t;

typedef struct
{
    uint32_t result;
    uint32_t phase;
    uint32_t failure_mask;
    uint32_t key_raw;
    uint32_t key_pin_mv;
    uint32_t key_sequence;
    uint32_t key_draw_count;
    uint32_t picture_displayed;
    uint32_t spi_kernel_clock_hz;
    uint32_t spi_sclk_hz;
    uint32_t dma_transfer_count;
    uint32_t transmitted_byte_count;
    uint32_t bus_error_count;
    uint32_t driver_error_count;
    uint32_t initialization_command_count;
    uint32_t completed_operation_count;
    uint32_t transmitted_pixel_count;
    uint32_t elapsed_ms;
} lcd_test_status_t;

typedef struct
{
    uint32_t start_time_ms;
    uint32_t next_key_refresh_ms;
    uint32_t pending_operation_count;
    uint32_t pending_key_raw;
    uint32_t pending_key_sequence;
} lcd_test_context_t;

typedef const uint8_t *(*lcd_ascii_font_getter_t)(uint8_t character);

static volatile lcd_test_status_t lcd_test_status;
static lcd_test_context_t lcd_test_context;
static uint8_t lcd_test_render_buffer[LCD_TEST_RENDER_BUFFER_SIZE];

static void lcd_test_fail(uint32_t failure)
{
    lcd_test_status.failure_mask |= failure;
    lcd_test_status.phase = LCD_TEST_PHASE_FAILED;
    lcd_test_status.result = LCD_TEST_RESULT_FAILED;
}

static void lcd_test_write_pixel(uint32_t pixel_index, uint16_t color)
{
    lcd_test_render_buffer[pixel_index * 2U] = (uint8_t)(color >> 8U);
    lcd_test_render_buffer[(pixel_index * 2U) + 1U] = (uint8_t)color;
}

static bool lcd_test_render_ascii(const char *text,
                                  uint32_t character_width,
                                  uint32_t character_height,
                                  lcd_ascii_font_getter_t font_getter,
                                  uint16_t foreground,
                                  uint16_t background,
                                  uint16_t *rendered_width)
{
    const uint8_t *glyph;
    uint32_t text_length = 0U;
    uint32_t bytes_per_row = (character_width + 7U) / 8U;
    uint32_t character_index;
    uint32_t row;
    uint32_t column;
    uint32_t output_width;
    uint32_t pixel_index;

    if ((text == NULL) || (font_getter == NULL) || (rendered_width == NULL))
    {
        return false;
    }
    while (text[text_length] != '\0')
    {
        text_length++;
    }
    output_width = text_length * character_width;
    if ((text_length == 0U) ||
        ((output_width * character_height * 2U) > LCD_TEST_RENDER_BUFFER_SIZE))
    {
        return false;
    }

    for (character_index = 0U; character_index < text_length; character_index++)
    {
        glyph = font_getter((uint8_t)text[character_index]);
        if (glyph == NULL)
        {
            return false;
        }
        for (row = 0U; row < character_height; row++)
        {
            for (column = 0U; column < character_width; column++)
            {
                pixel_index = (row * output_width) +
                              (character_index * character_width) + column;
                lcd_test_write_pixel(
                    pixel_index,
                    ((glyph[(row * bytes_per_row) + (column / 8U)] &
                      (1U << (column % 8U))) != 0U) ? foreground : background);
            }
        }
    }

    *rendered_width = (uint16_t)output_width;
    return true;
}

static bool lcd_test_render_chinese(uint16_t foreground, uint16_t background)
{
    const uint8_t *glyph;
    uint32_t glyph_index;
    uint32_t row;
    uint32_t column;
    uint32_t output_width = LCD_FONT_CHINESE32_WIDTH * LCD_FONT_CHINESE32_COUNT;
    uint32_t pixel_index;

    if ((output_width * LCD_FONT_CHINESE32_HEIGHT * 2U) >
        LCD_TEST_RENDER_BUFFER_SIZE)
    {
        return false;
    }

    for (glyph_index = 0U; glyph_index < LCD_FONT_CHINESE32_COUNT; glyph_index++)
    {
        glyph = lcd_font_get_chinese32(glyph_index);
        if (glyph == NULL)
        {
            return false;
        }
        for (row = 0U; row < LCD_FONT_CHINESE32_HEIGHT; row++)
        {
            for (column = 0U; column < LCD_FONT_CHINESE32_WIDTH; column++)
            {
                pixel_index = (row * output_width) +
                              (glyph_index * LCD_FONT_CHINESE32_WIDTH) + column;
                lcd_test_write_pixel(
                    pixel_index,
                    ((glyph[(row * 4U) + (column / 8U)] &
                      (1U << (column % 8U))) != 0U) ? foreground : background);
            }
        }
    }
    return true;
}

static bool lcd_test_render_key_value(uint32_t raw)
{
    char text[6] = {' ', ' ', ' ', ' ', '0', '\0'};
    uint32_t position = 4U;
    uint16_t rendered_width;

    while (raw > 0U)
    {
        text[position] = (char)('0' + (raw % 10U));
        raw /= 10U;
        if (position == 0U)
        {
            break;
        }
        position--;
    }

    return lcd_test_render_ascii(text,
                                 LCD_FONT_ASCII32_WIDTH,
                                 LCD_FONT_ASCII32_HEIGHT,
                                 lcd_font_get_ascii32,
                                 0xFFFFU,
                                 0x0000U,
                                 &rendered_width) &&
           (rendered_width == 80U);
}

static bool lcd_test_capture_status(lcd_status_t *driver_status,
                                    bsp_lcd_status_t *bus_status)
{
    if (!lcd_get_status(driver_status) || !bsp_lcd_get_status(bus_status))
    {
        lcd_test_fail(LCD_TEST_FAILURE_DRIVER);
        return false;
    }

    lcd_test_status.spi_kernel_clock_hz = bus_status->spi_kernel_clock_hz;
    lcd_test_status.spi_sclk_hz = bus_status->spi_sclk_hz;
    lcd_test_status.dma_transfer_count = bus_status->dma_transfer_count;
    lcd_test_status.transmitted_byte_count = bus_status->transmitted_byte_count;
    lcd_test_status.bus_error_count = bus_status->error_count;
    lcd_test_status.driver_error_count = driver_status->error_count;
    lcd_test_status.initialization_command_count =
        driver_status->initialization_command_count;
    lcd_test_status.completed_operation_count = driver_status->completed_operation_count;
    lcd_test_status.transmitted_pixel_count = driver_status->transmitted_pixel_count;
    return true;
}

static void lcd_test_check_initial_counters(void)
{
    if ((lcd_test_status.dma_transfer_count != LCD_TEST_INITIAL_DMA_TRANSFERS) ||
        (lcd_test_status.transmitted_byte_count != LCD_TEST_INITIAL_BYTES) ||
        (lcd_test_status.bus_error_count != 0U) ||
        (lcd_test_status.driver_error_count != 0U) ||
        (lcd_test_status.initialization_command_count != LCD_TEST_INITIAL_COMMANDS) ||
        (lcd_test_status.completed_operation_count != LCD_TEST_INITIAL_OPERATIONS) ||
        (lcd_test_status.transmitted_pixel_count != LCD_TEST_INITIAL_PIXELS) ||
        (lcd_test_status.picture_displayed != 1U) ||
        (lcd_test_status.key_draw_count != 1U))
    {
        lcd_test_fail(LCD_TEST_FAILURE_COUNTERS);
        return;
    }

    lcd_test_status.result = LCD_TEST_RESULT_PASSED;
}

static bool lcd_test_draw_render_buffer(uint16_t x,
                                        uint16_t y,
                                        uint16_t width,
                                        uint16_t height)
{
    return lcd_draw_rgb565_image(x, y, width, height,
                                 lcd_test_render_buffer,
                                 (uint32_t)width * height * 2U);
}

void lcd_test_init(uint32_t current_time_ms)
{
    lcd_test_status.result = LCD_TEST_RESULT_RUNNING;
    lcd_test_status.phase = LCD_TEST_PHASE_WAIT_INITIALIZATION;
    lcd_test_status.failure_mask = 0U;
    lcd_test_status.key_raw = 0U;
    lcd_test_status.key_pin_mv = 0U;
    lcd_test_status.key_sequence = 0U;
    lcd_test_status.key_draw_count = 0U;
    lcd_test_status.picture_displayed = 0U;
    lcd_test_status.spi_kernel_clock_hz = 0U;
    lcd_test_status.spi_sclk_hz = 0U;
    lcd_test_status.dma_transfer_count = 0U;
    lcd_test_status.transmitted_byte_count = 0U;
    lcd_test_status.bus_error_count = 0U;
    lcd_test_status.driver_error_count = 0U;
    lcd_test_status.initialization_command_count = 0U;
    lcd_test_status.completed_operation_count = 0U;
    lcd_test_status.transmitted_pixel_count = 0U;
    lcd_test_status.elapsed_ms = 0U;
    lcd_test_context.start_time_ms = current_time_ms;
    lcd_test_context.next_key_refresh_ms = current_time_ms;
    lcd_test_context.pending_operation_count = 0U;
    lcd_test_context.pending_key_raw = 0U;
    lcd_test_context.pending_key_sequence = 0U;

    if (!lcd_init(current_time_ms))
    {
        lcd_test_fail(LCD_TEST_FAILURE_INITIALIZATION);
    }
}

void lcd_test_process(uint32_t current_time_ms)
{
    lcd_status_t driver_status = {0U};
    bsp_lcd_status_t bus_status = {0U};
    uint32_t key_raw;
    uint32_t key_sequence;
    uint16_t rendered_width;

    if ((lcd_test_status.result == LCD_TEST_RESULT_IDLE) ||
        (lcd_test_status.result == LCD_TEST_RESULT_FAILED))
    {
        return;
    }

    lcd_test_status.elapsed_ms = current_time_ms - lcd_test_context.start_time_ms;
    if ((lcd_test_status.result == LCD_TEST_RESULT_RUNNING) &&
        (lcd_test_status.elapsed_ms >= BOARD_LCD_TEST_TIMEOUT_MS))
    {
        lcd_test_fail(LCD_TEST_FAILURE_TIMEOUT);
        return;
    }

    lcd_process(current_time_ms);
    if (!lcd_test_capture_status(&driver_status, &bus_status))
    {
        return;
    }
    if ((driver_status.error_count != 0U) || (bus_status.error_count != 0U))
    {
        lcd_test_fail(LCD_TEST_FAILURE_DRIVER);
        return;
    }

    switch (lcd_test_status.phase)
    {
        case LCD_TEST_PHASE_WAIT_INITIALIZATION:
            if (driver_status.ready && !driver_status.busy)
            {
                lcd_test_status.phase = LCD_TEST_PHASE_CLEAR;
            }
            break;

        case LCD_TEST_PHASE_CLEAR:
            if (!lcd_fill_rect(0U, 0U,
                               (uint16_t)(BOARD_LCD_WIDTH - 1U),
                               (uint16_t)(BOARD_LCD_HEIGHT - 1U),
                               0x0000U))
            {
                lcd_test_fail(LCD_TEST_FAILURE_DRIVER);
                break;
            }
            lcd_test_status.phase = LCD_TEST_PHASE_WAIT_CLEAR;
            break;

        case LCD_TEST_PHASE_WAIT_CLEAR:
            if ((driver_status.completed_operation_count >= 1U) && !driver_status.busy)
            {
                lcd_test_status.phase = LCD_TEST_PHASE_DRAW_DMBOT;
            }
            break;

        case LCD_TEST_PHASE_DRAW_DMBOT:
            if (!lcd_test_render_ascii("dmBot",
                                       LCD_FONT_ASCII24_WIDTH,
                                       LCD_FONT_ASCII24_HEIGHT,
                                       lcd_font_get_ascii24,
                                       0xFC07U,
                                       0x0000U,
                                       &rendered_width) ||
                !lcd_test_draw_render_buffer(120U, 72U, rendered_width, 24U))
            {
                lcd_test_fail(LCD_TEST_FAILURE_FONT);
                break;
            }
            lcd_test_status.phase = LCD_TEST_PHASE_WAIT_DMBOT;
            break;

        case LCD_TEST_PHASE_WAIT_DMBOT:
            if ((driver_status.completed_operation_count >= 2U) && !driver_status.busy)
            {
                lcd_test_status.phase = LCD_TEST_PHASE_DRAW_CHINESE;
            }
            break;

        case LCD_TEST_PHASE_DRAW_CHINESE:
            if (!lcd_test_render_chinese(0xFFFFU, 0x0000U) ||
                !lcd_test_draw_render_buffer(84U, 100U, 128U, 32U))
            {
                lcd_test_fail(LCD_TEST_FAILURE_FONT);
                break;
            }
            lcd_test_status.phase = LCD_TEST_PHASE_WAIT_CHINESE;
            break;

        case LCD_TEST_PHASE_WAIT_CHINESE:
            if ((driver_status.completed_operation_count >= 3U) && !driver_status.busy)
            {
                lcd_test_status.phase = LCD_TEST_PHASE_DRAW_LEFT_LINE;
            }
            break;

        case LCD_TEST_PHASE_DRAW_LEFT_LINE:
            if (!lcd_fill_rect(10U, 0U, 10U,
                               (uint16_t)(BOARD_LCD_HEIGHT - 1U),
                               0xFFFFU))
            {
                lcd_test_fail(LCD_TEST_FAILURE_DRIVER);
                break;
            }
            lcd_test_status.phase = LCD_TEST_PHASE_WAIT_LEFT_LINE;
            break;

        case LCD_TEST_PHASE_WAIT_LEFT_LINE:
            if ((driver_status.completed_operation_count >= 4U) && !driver_status.busy)
            {
                lcd_test_status.phase = LCD_TEST_PHASE_DRAW_RIGHT_LINE;
            }
            break;

        case LCD_TEST_PHASE_DRAW_RIGHT_LINE:
            if (!lcd_fill_rect(270U, 0U, 270U,
                               (uint16_t)(BOARD_LCD_HEIGHT - 1U),
                               0xFFFFU))
            {
                lcd_test_fail(LCD_TEST_FAILURE_DRIVER);
                break;
            }
            lcd_test_status.phase = LCD_TEST_PHASE_WAIT_RIGHT_LINE;
            break;

        case LCD_TEST_PHASE_WAIT_RIGHT_LINE:
            if ((driver_status.completed_operation_count >= 5U) && !driver_status.busy)
            {
                lcd_test_status.phase = LCD_TEST_PHASE_DRAW_IMAGE;
            }
            break;

        case LCD_TEST_PHASE_DRAW_IMAGE:
            if (!lcd_draw_rgb565_image(180U, 150U,
                                       LCD_PICTURE_WIDTH,
                                       LCD_PICTURE_HEIGHT,
                                       lcd_picture_data,
                                       LCD_PICTURE_DATA_SIZE))
            {
                lcd_test_fail(LCD_TEST_FAILURE_DRIVER);
                break;
            }
            lcd_test_status.phase = LCD_TEST_PHASE_WAIT_IMAGE;
            break;

        case LCD_TEST_PHASE_WAIT_IMAGE:
            if ((driver_status.completed_operation_count >= 6U) && !driver_status.busy)
            {
                lcd_test_status.picture_displayed = 1U;
                lcd_test_status.phase = LCD_TEST_PHASE_DRAW_KEY_VALUE;
            }
            break;

        case LCD_TEST_PHASE_DRAW_KEY_VALUE:
            if (!key_test_get_latest(&key_raw, &key_sequence))
            {
                break;
            }
            if (!lcd_test_render_key_value(key_raw) ||
                !lcd_test_draw_render_buffer(50U, 170U, 80U, 32U))
            {
                lcd_test_fail(LCD_TEST_FAILURE_FONT);
                break;
            }
            lcd_test_context.pending_operation_count =
                driver_status.completed_operation_count + 1U;
            lcd_test_context.pending_key_raw = key_raw;
            lcd_test_context.pending_key_sequence = key_sequence;
            lcd_test_status.phase = LCD_TEST_PHASE_WAIT_KEY_VALUE;
            break;

        case LCD_TEST_PHASE_WAIT_KEY_VALUE:
            if ((driver_status.completed_operation_count >=
                 lcd_test_context.pending_operation_count) && !driver_status.busy)
            {
                lcd_test_status.key_raw = lcd_test_context.pending_key_raw;
                lcd_test_status.key_pin_mv =
                    (uint32_t)((((uint64_t)lcd_test_status.key_raw *
                                 BOARD_ADC_REFERENCE_MV) +
                                (BOARD_ADC_12BIT_FULL_SCALE / 2U)) /
                               BOARD_ADC_12BIT_FULL_SCALE);
                lcd_test_status.key_sequence = lcd_test_context.pending_key_sequence;
                lcd_test_status.key_draw_count++;
                lcd_test_context.next_key_refresh_ms =
                    current_time_ms + BOARD_KEY_ADC_DISPLAY_REFRESH_MS;
                lcd_test_status.phase = LCD_TEST_PHASE_ACTIVE;
                if (lcd_test_status.result == LCD_TEST_RESULT_RUNNING)
                {
                    if (lcd_test_capture_status(&driver_status, &bus_status))
                    {
                        lcd_test_check_initial_counters();
                    }
                }
            }
            break;

        /* 固定画面显示完后，仍继续刷新模拟按键的读数，方便实际操作并观察屏幕变化。 */
        case LCD_TEST_PHASE_ACTIVE:
            if (((int32_t)(current_time_ms -
                           lcd_test_context.next_key_refresh_ms) >= 0) &&
                key_test_get_latest(&key_raw, &key_sequence) &&
                (key_sequence != lcd_test_status.key_sequence))
            {
                lcd_test_status.phase = LCD_TEST_PHASE_DRAW_KEY_VALUE;
            }
            break;

        case LCD_TEST_PHASE_IDLE:
        case LCD_TEST_PHASE_FAILED:
        default:
            break;
    }
}
