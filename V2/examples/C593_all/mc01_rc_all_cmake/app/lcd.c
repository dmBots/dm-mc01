#include "lcd.h"

#include <stddef.h>

#include "board_config.h"
#include "bsp_lcd.h"

#define LCD_COMMAND_SLEEP_OUT       (0x11U)
#define LCD_COMMAND_COLUMN_ADDRESS  (0x2AU)
#define LCD_COMMAND_ROW_ADDRESS     (0x2BU)
#define LCD_COMMAND_MEMORY_WRITE    (0x2CU)

#define LCD_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

typedef struct
{
    uint8_t command;
    const uint8_t *data;
    uint8_t data_size;
} lcd_initialization_entry_t;

typedef enum
{
    LCD_PHASE_DISABLED = 0U,
    LCD_PHASE_RESET_ASSERTED,
    LCD_PHASE_RESET_RELEASED,
    LCD_PHASE_BACKLIGHT_DELAY,
    LCD_PHASE_SLEEP_COMMAND,
    LCD_PHASE_SLEEP_DELAY,
    LCD_PHASE_INITIALIZATION_COMMAND,
    LCD_PHASE_INITIALIZATION_DATA,
    LCD_PHASE_READY,
    LCD_PHASE_COLUMN_COMMAND,
    LCD_PHASE_COLUMN_DATA,
    LCD_PHASE_ROW_COMMAND,
    LCD_PHASE_ROW_DATA,
    LCD_PHASE_MEMORY_WRITE_COMMAND,
    LCD_PHASE_PIXEL_DATA,
    LCD_PHASE_FAULT
} lcd_phase_t;

typedef struct
{
    bool initialized;
    bool ready;
    bool transfer_pending;
    bool transfer_is_initialization_command;
    lcd_phase_t phase;
    lcd_phase_t phase_after_transfer;
    uint32_t delay_until_ms;
    uint32_t transfer_start_time_ms;
    uint32_t transfer_pixel_count;
    uint32_t initialization_index;
    uint32_t remaining_pixel_count;
    uint32_t initialization_command_count;
    uint32_t completed_operation_count;
    uint32_t transmitted_pixel_count;
    uint32_t error_count;
    const uint8_t *pixel_source;
    uint32_t pixel_source_offset_bytes;
    uint8_t column_data[4];
    uint8_t row_data[4];
    uint8_t pixel_data[BOARD_LCD_DMA_BUFFER_SIZE];
} lcd_context_t;

static const uint8_t lcd_madctl[] = {0x70U};
static const uint8_t lcd_colmod[] = {0x05U};
static const uint8_t lcd_porctrl[] = {0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U};
static const uint8_t lcd_gctrl[] = {0x35U};
static const uint8_t lcd_vcoms[] = {0x32U};
static const uint8_t lcd_lcmctrl[] = {0x01U};
static const uint8_t lcd_vrhs[] = {0x15U};
static const uint8_t lcd_vdvs[] = {0x20U};
static const uint8_t lcd_frctrl2[] = {0x0FU};
static const uint8_t lcd_pwctrl1[] = {0xA4U, 0xA1U};
static const uint8_t lcd_pvgamctrl[] = {
    0xD0U, 0x08U, 0x0EU, 0x09U, 0x09U, 0x05U, 0x31U,
    0x33U, 0x48U, 0x17U, 0x14U, 0x15U, 0x31U, 0x34U
};
static const uint8_t lcd_nvgamctrl[] = {
    0xD0U, 0x08U, 0x0EU, 0x09U, 0x09U, 0x15U, 0x31U,
    0x33U, 0x48U, 0x17U, 0x14U, 0x15U, 0x31U, 0x34U
};

static const lcd_initialization_entry_t lcd_initialization_table[] = {
    {0x36U, lcd_madctl, sizeof(lcd_madctl)},
    {0x3AU, lcd_colmod, sizeof(lcd_colmod)},
    {0xB2U, lcd_porctrl, sizeof(lcd_porctrl)},
    {0xB7U, lcd_gctrl, sizeof(lcd_gctrl)},
    {0xBBU, lcd_vcoms, sizeof(lcd_vcoms)},
    {0xC2U, lcd_lcmctrl, sizeof(lcd_lcmctrl)},
    {0xC3U, lcd_vrhs, sizeof(lcd_vrhs)},
    {0xC4U, lcd_vdvs, sizeof(lcd_vdvs)},
    {0xC6U, lcd_frctrl2, sizeof(lcd_frctrl2)},
    {0xD0U, lcd_pwctrl1, sizeof(lcd_pwctrl1)},
    {0xE0U, lcd_pvgamctrl, sizeof(lcd_pvgamctrl)},
    {0xE1U, lcd_nvgamctrl, sizeof(lcd_nvgamctrl)},
    {0x21U, NULL, 0U},
    {0x29U, NULL, 0U}
};

static const uint8_t lcd_sleep_out_command = LCD_COMMAND_SLEEP_OUT;
static const uint8_t lcd_column_address_command = LCD_COMMAND_COLUMN_ADDRESS;
static const uint8_t lcd_row_address_command = LCD_COMMAND_ROW_ADDRESS;
static const uint8_t lcd_memory_write_command = LCD_COMMAND_MEMORY_WRITE;

static lcd_context_t lcd_context;

static bool lcd_time_reached(uint32_t current_time_ms, uint32_t deadline_ms)
{
    return (int32_t)(current_time_ms - deadline_ms) >= 0;
}

static void lcd_fail(void)
{
    lcd_context.ready = false;
    lcd_context.transfer_pending = false;
    lcd_context.transfer_pixel_count = 0U;
    lcd_context.error_count++;
    lcd_context.phase = LCD_PHASE_FAULT;
}

static bool lcd_start_transfer(bsp_lcd_transfer_type_t type,
                               const uint8_t *data,
                               uint32_t size,
                               lcd_phase_t phase_after_transfer,
                               bool is_initialization_command,
                               uint32_t pixel_count,
                               uint32_t current_time_ms)
{
    if (!bsp_lcd_transmit_dma(type, data, size))
    {
        lcd_fail();
        return false;
    }

    lcd_context.transfer_pending = true;
    lcd_context.transfer_is_initialization_command = is_initialization_command;
    lcd_context.phase_after_transfer = phase_after_transfer;
    lcd_context.transfer_start_time_ms = current_time_ms;
    lcd_context.transfer_pixel_count = pixel_count;
    return true;
}

static bool lcd_process_transfer_result(uint32_t current_time_ms)
{
    bool success;

    if (!lcd_context.transfer_pending)
    {
        return true;
    }

    if (!bsp_lcd_take_transfer_result(&success))
    {
        if ((current_time_ms - lcd_context.transfer_start_time_ms) >=
            BOARD_LCD_TRANSFER_TIMEOUT_MS)
        {
            lcd_fail();
        }
        return false;
    }

    lcd_context.transfer_pending = false;
    if (!success)
    {
        lcd_fail();
        return false;
    }

    if (lcd_context.transfer_is_initialization_command)
    {
        lcd_context.initialization_command_count++;
    }
    if (lcd_context.transfer_pixel_count > 0U)
    {
        lcd_context.remaining_pixel_count -= lcd_context.transfer_pixel_count;
        lcd_context.transmitted_pixel_count += lcd_context.transfer_pixel_count;
        if (lcd_context.pixel_source != NULL)
        {
            lcd_context.pixel_source_offset_bytes +=
                lcd_context.transfer_pixel_count * 2U;
        }
        lcd_context.transfer_pixel_count = 0U;
    }

    lcd_context.phase = lcd_context.phase_after_transfer;
    if (lcd_context.phase == LCD_PHASE_SLEEP_DELAY)
    {
        lcd_context.delay_until_ms = current_time_ms + BOARD_LCD_SLEEP_OUT_MS;
    }
    return true;
}

bool lcd_init(uint32_t current_time_ms)
{
    if (!bsp_lcd_init())
    {
        return false;
    }

    lcd_context.initialized = true;
    lcd_context.ready = false;
    lcd_context.transfer_pending = false;
    lcd_context.transfer_is_initialization_command = false;
    lcd_context.phase = LCD_PHASE_RESET_ASSERTED;
    lcd_context.phase_after_transfer = LCD_PHASE_DISABLED;
    lcd_context.delay_until_ms = current_time_ms + BOARD_LCD_RESET_PULSE_MS;
    lcd_context.transfer_start_time_ms = 0U;
    lcd_context.transfer_pixel_count = 0U;
    lcd_context.initialization_index = 0U;
    lcd_context.remaining_pixel_count = 0U;
    lcd_context.initialization_command_count = 0U;
    lcd_context.completed_operation_count = 0U;
    lcd_context.transmitted_pixel_count = 0U;
    lcd_context.error_count = 0U;
    lcd_context.pixel_source = NULL;
    lcd_context.pixel_source_offset_bytes = 0U;

    bsp_lcd_set_backlight(false);
    bsp_lcd_set_reset(true);
    return true;
}

void lcd_process(uint32_t current_time_ms)
{
    const lcd_initialization_entry_t *entry;
    const uint8_t *pixel_data;
    uint32_t pixel_count;

    if (!lcd_context.initialized || (lcd_context.phase == LCD_PHASE_FAULT))
    {
        return;
    }

    bsp_lcd_process();
    if (!lcd_process_transfer_result(current_time_ms))
    {
        return;
    }

    switch (lcd_context.phase)
    {
        case LCD_PHASE_RESET_ASSERTED:
            if (lcd_time_reached(current_time_ms, lcd_context.delay_until_ms))
            {
                bsp_lcd_set_reset(false);
                lcd_context.delay_until_ms = current_time_ms + BOARD_LCD_RESET_RECOVERY_MS;
                lcd_context.phase = LCD_PHASE_RESET_RELEASED;
            }
            break;

        case LCD_PHASE_RESET_RELEASED:
            if (lcd_time_reached(current_time_ms, lcd_context.delay_until_ms))
            {
                bsp_lcd_set_backlight(true);
                lcd_context.delay_until_ms = current_time_ms + BOARD_LCD_BACKLIGHT_STABILIZE_MS;
                lcd_context.phase = LCD_PHASE_BACKLIGHT_DELAY;
            }
            break;

        case LCD_PHASE_BACKLIGHT_DELAY:
            if (lcd_time_reached(current_time_ms, lcd_context.delay_until_ms))
            {
                lcd_context.phase = LCD_PHASE_SLEEP_COMMAND;
            }
            break;

        case LCD_PHASE_SLEEP_COMMAND:
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_COMMAND,
                                     &lcd_sleep_out_command,
                                     sizeof(lcd_sleep_out_command),
                                     LCD_PHASE_SLEEP_DELAY,
                                     true,
                                     0U,
                                     current_time_ms);
            break;

        case LCD_PHASE_SLEEP_DELAY:
            if (lcd_time_reached(current_time_ms, lcd_context.delay_until_ms))
            {
                lcd_context.phase = LCD_PHASE_INITIALIZATION_COMMAND;
            }
            break;

        case LCD_PHASE_INITIALIZATION_COMMAND:
            if (lcd_context.initialization_index >= LCD_ARRAY_SIZE(lcd_initialization_table))
            {
                lcd_context.ready = true;
                lcd_context.phase = LCD_PHASE_READY;
                break;
            }
            entry = &lcd_initialization_table[lcd_context.initialization_index];
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_COMMAND,
                                     &entry->command,
                                     sizeof(entry->command),
                                     LCD_PHASE_INITIALIZATION_DATA,
                                     true,
                                     0U,
                                     current_time_ms);
            break;

        case LCD_PHASE_INITIALIZATION_DATA:
            entry = &lcd_initialization_table[lcd_context.initialization_index];
            if (entry->data_size == 0U)
            {
                lcd_context.initialization_index++;
                lcd_context.phase = LCD_PHASE_INITIALIZATION_COMMAND;
                break;
            }
            lcd_context.initialization_index++;
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_DATA,
                                     entry->data,
                                     entry->data_size,
                                     LCD_PHASE_INITIALIZATION_COMMAND,
                                     false,
                                     0U,
                                     current_time_ms);
            break;

        case LCD_PHASE_COLUMN_COMMAND:
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_COMMAND,
                                     &lcd_column_address_command,
                                     sizeof(lcd_column_address_command),
                                     LCD_PHASE_COLUMN_DATA,
                                     false,
                                     0U,
                                     current_time_ms);
            break;

        case LCD_PHASE_COLUMN_DATA:
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_DATA,
                                     lcd_context.column_data,
                                     sizeof(lcd_context.column_data),
                                     LCD_PHASE_ROW_COMMAND,
                                     false,
                                     0U,
                                     current_time_ms);
            break;

        case LCD_PHASE_ROW_COMMAND:
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_COMMAND,
                                     &lcd_row_address_command,
                                     sizeof(lcd_row_address_command),
                                     LCD_PHASE_ROW_DATA,
                                     false,
                                     0U,
                                     current_time_ms);
            break;

        case LCD_PHASE_ROW_DATA:
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_DATA,
                                     lcd_context.row_data,
                                     sizeof(lcd_context.row_data),
                                     LCD_PHASE_MEMORY_WRITE_COMMAND,
                                     false,
                                     0U,
                                     current_time_ms);
            break;

        case LCD_PHASE_MEMORY_WRITE_COMMAND:
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_COMMAND,
                                     &lcd_memory_write_command,
                                     sizeof(lcd_memory_write_command),
                                     LCD_PHASE_PIXEL_DATA,
                                     false,
                                     0U,
                                     current_time_ms);
            break;

        /* 图片分成小块逐次发给屏幕，两块之间先返回主循环；整张图画完前不能改动图片数组。 */
        case LCD_PHASE_PIXEL_DATA:
            if (lcd_context.remaining_pixel_count == 0U)
            {
                lcd_context.completed_operation_count++;
                lcd_context.pixel_source = NULL;
                lcd_context.pixel_source_offset_bytes = 0U;
                lcd_context.phase = LCD_PHASE_READY;
                break;
            }
            pixel_count = lcd_context.remaining_pixel_count;
            if (pixel_count > (BOARD_LCD_DMA_BUFFER_SIZE / 2U))
            {
                pixel_count = BOARD_LCD_DMA_BUFFER_SIZE / 2U;
            }
            pixel_data = lcd_context.pixel_data;
            if (lcd_context.pixel_source != NULL)
            {
                pixel_data = &lcd_context.pixel_source[lcd_context.pixel_source_offset_bytes];
            }
            (void)lcd_start_transfer(BSP_LCD_TRANSFER_DATA,
                                     pixel_data,
                                     pixel_count * 2U,
                                     LCD_PHASE_PIXEL_DATA,
                                     false,
                                     pixel_count,
                                     current_time_ms);
            break;

        case LCD_PHASE_DISABLED:
        case LCD_PHASE_READY:
        case LCD_PHASE_FAULT:
        default:
            break;
    }
}

bool lcd_fill_rect(uint16_t x_start,
                   uint16_t y_start,
                   uint16_t x_end,
                   uint16_t y_end,
                   uint16_t color)
{
    uint32_t index;
    uint16_t controller_x_start;
    uint16_t controller_x_end;
    uint16_t controller_y_start;
    uint16_t controller_y_end;

    if (!lcd_context.initialized || !lcd_context.ready ||
        (lcd_context.phase != LCD_PHASE_READY) ||
        (x_start > x_end) || (y_start > y_end) ||
        (x_end >= BOARD_LCD_WIDTH) || (y_end >= BOARD_LCD_HEIGHT))
    {
        return false;
    }

    controller_x_start = (uint16_t)(x_start + BOARD_LCD_X_OFFSET);
    controller_x_end = (uint16_t)(x_end + BOARD_LCD_X_OFFSET);
    controller_y_start = (uint16_t)(y_start + BOARD_LCD_Y_OFFSET);
    controller_y_end = (uint16_t)(y_end + BOARD_LCD_Y_OFFSET);

    lcd_context.column_data[0] = (uint8_t)(controller_x_start >> 8U);
    lcd_context.column_data[1] = (uint8_t)controller_x_start;
    lcd_context.column_data[2] = (uint8_t)(controller_x_end >> 8U);
    lcd_context.column_data[3] = (uint8_t)controller_x_end;
    lcd_context.row_data[0] = (uint8_t)(controller_y_start >> 8U);
    lcd_context.row_data[1] = (uint8_t)controller_y_start;
    lcd_context.row_data[2] = (uint8_t)(controller_y_end >> 8U);
    lcd_context.row_data[3] = (uint8_t)controller_y_end;

    for (index = 0U; index < BOARD_LCD_DMA_BUFFER_SIZE; index += 2U)
    {
        lcd_context.pixel_data[index] = (uint8_t)(color >> 8U);
        lcd_context.pixel_data[index + 1U] = (uint8_t)color;
    }

    lcd_context.remaining_pixel_count =
        ((uint32_t)x_end - x_start + 1U) * ((uint32_t)y_end - y_start + 1U);
    lcd_context.pixel_source = NULL;
    lcd_context.pixel_source_offset_bytes = 0U;
    lcd_context.ready = true;
    lcd_context.phase = LCD_PHASE_COLUMN_COMMAND;
    return true;
}

bool lcd_draw_rgb565_image(uint16_t x,
                           uint16_t y,
                           uint16_t width,
                           uint16_t height,
                           const uint8_t *pixel_data,
                           uint32_t pixel_data_size)
{
    uint32_t pixel_count;
    uint16_t x_end;
    uint16_t y_end;
    uint16_t controller_x_start;
    uint16_t controller_x_end;
    uint16_t controller_y_start;
    uint16_t controller_y_end;

    pixel_count = (uint32_t)width * height;
    if (!lcd_context.initialized || !lcd_context.ready ||
        (lcd_context.phase != LCD_PHASE_READY) ||
        (pixel_data == NULL) || (width == 0U) || (height == 0U) ||
        (x >= BOARD_LCD_WIDTH) || (y >= BOARD_LCD_HEIGHT) ||
        (width > (BOARD_LCD_WIDTH - x)) || (height > (BOARD_LCD_HEIGHT - y)) ||
        (pixel_data_size != (pixel_count * 2U)))
    {
        return false;
    }

    x_end = (uint16_t)(x + width - 1U);
    y_end = (uint16_t)(y + height - 1U);
    controller_x_start = (uint16_t)(x + BOARD_LCD_X_OFFSET);
    controller_x_end = (uint16_t)(x_end + BOARD_LCD_X_OFFSET);
    controller_y_start = (uint16_t)(y + BOARD_LCD_Y_OFFSET);
    controller_y_end = (uint16_t)(y_end + BOARD_LCD_Y_OFFSET);

    lcd_context.column_data[0] = (uint8_t)(controller_x_start >> 8U);
    lcd_context.column_data[1] = (uint8_t)controller_x_start;
    lcd_context.column_data[2] = (uint8_t)(controller_x_end >> 8U);
    lcd_context.column_data[3] = (uint8_t)controller_x_end;
    lcd_context.row_data[0] = (uint8_t)(controller_y_start >> 8U);
    lcd_context.row_data[1] = (uint8_t)controller_y_start;
    lcd_context.row_data[2] = (uint8_t)(controller_y_end >> 8U);
    lcd_context.row_data[3] = (uint8_t)controller_y_end;
    lcd_context.remaining_pixel_count = pixel_count;
    lcd_context.pixel_source = pixel_data;
    lcd_context.pixel_source_offset_bytes = 0U;
    lcd_context.phase = LCD_PHASE_COLUMN_COMMAND;
    return true;
}

bool lcd_get_status(lcd_status_t *status)
{
    if ((status == NULL) || !lcd_context.initialized)
    {
        return false;
    }

    status->ready = lcd_context.ready;
    status->busy = lcd_context.transfer_pending ||
                   ((lcd_context.phase != LCD_PHASE_READY) &&
                    (lcd_context.phase != LCD_PHASE_FAULT));
    status->initialization_command_count = lcd_context.initialization_command_count;
    status->completed_operation_count = lcd_context.completed_operation_count;
    status->transmitted_pixel_count = lcd_context.transmitted_pixel_count;
    status->error_count = lcd_context.error_count;
    return true;
}
