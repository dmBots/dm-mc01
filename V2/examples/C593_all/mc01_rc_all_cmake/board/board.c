#include "board.h"
#include "bsp_pwr.h"
#include "mx_system.h"
#include "stm32c5xx_hal_flash_itf.h"

/* mx_system_init() 在硬件准备完成后自动调用这里，再返回 main。
 * BOOT_SEL=1 表示用 BOOT0 引脚选择启动方式，正常运行例程时应保持 BOOT0 为低电平。
 * 已经是 1 时直接跳过；否则只修改这一位，保留读保护、启动地址等其他选项。
 * 首次写入后请完整断电再上电，让芯片重新加载设置；这里不强制重启。
 */
system_status_t post_system_init_hook(void)
{
    hal_status_t status;

    if (HAL_FLASH_ITF_OB_GetBootSelection(HAL_FLASH) == HAL_FLASH_ITF_OB_BOOT_PIN)
    {
        return SYSTEM_OK;
    }

    if (HAL_FLASH_ITF_OB_Unlock(HAL_FLASH) != HAL_OK)
    {
        return SYSTEM_POSTSYSTEM_ERROR;
    }

    status = HAL_FLASH_ITF_OB_SetBootSelection(HAL_FLASH, HAL_FLASH_ITF_OB_BOOT_PIN);
    if (status == HAL_OK)
    {
        status = HAL_FLASH_ITF_OB_Program(HAL_FLASH);
    }

    /* 无论写入是否成功，都重新锁住选项字节，防止后续代码误改。 */
    if (HAL_FLASH_ITF_OB_Lock(HAL_FLASH) != HAL_OK)
    {
        return SYSTEM_POSTSYSTEM_ERROR;
    }

    return (status == HAL_OK) ? SYSTEM_OK : SYSTEM_POSTSYSTEM_ERROR;
}

/* 上电后先给外设供电，但暂不加热；稍后由 imu_heater 根据测试流程打开加热。 */
void board_init()
{
    bsp_pwr_outer_5v_cmd(true);
    bsp_pwr_heat_5v_cmd(false);
}
