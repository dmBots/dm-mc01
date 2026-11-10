/*
 * 读取电池电压和模拟按键电压：ADC 将电压变成数字，DMA 自动把数字存入内存。
 * VBAT（电池电压）使用 ADC1，模拟按键使用 ADC2，两路分别操作。
 * 先调用 init 做准备，再 start_sample 开始一次测量；循环调用 process，直到 read_sample 返回 true。
 * raw 是本次测量的原始数字，sequence 是样本编号，每测得一个新样本加 1；同一样本只能取一次。
 */
#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool busy;
    bool sample_ready;
    uint32_t latest_raw;
    uint32_t sample_sequence;
    uint32_t dma_complete_count;
    uint32_t error_count;
} bsp_adc_vbat_status_t;

typedef bsp_adc_vbat_status_t bsp_adc_key_status_t;

bool bsp_adc_vbat_init(void);
void bsp_adc_vbat_process(void);
/* 先等上次测量结束并用 read_sample 取走结果，才能开始下一次测量；key 接口用法相同。 */
bool bsp_adc_vbat_start_sample(void);
bool bsp_adc_vbat_read_sample(uint32_t *raw, uint32_t *sequence);
bool bsp_adc_vbat_get_status(bsp_adc_vbat_status_t *status);

bool bsp_adc_key_init(void);
void bsp_adc_key_process(void);
bool bsp_adc_key_start_sample(void);
bool bsp_adc_key_read_sample(uint32_t *raw, uint32_t *sequence);
bool bsp_adc_key_get_status(bsp_adc_key_status_t *status);

#endif
