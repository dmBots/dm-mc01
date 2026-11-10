#include "bsp_adc.h"

#include <stddef.h>

#include "mx_adc1.h"
#include "mx_adc2.h"

typedef struct
{
    uint32_t dma_sample;
    uint32_t latest_raw;
    uint32_t sample_sequence;
    uint32_t dma_complete_count;
    uint32_t error_count;
    bool initialized;
    volatile bool busy;
    volatile bool dma_complete_pending;
    volatile bool dma_error_pending;
    volatile bool stop_complete_pending;
    bool stop_requested;
    bool publish_after_stop;
    bool sample_ready;
} bsp_adc_context_t;

static bsp_adc_context_t vbat_context;
static bsp_adc_context_t key_context;

static bool bsp_adc_init_context(bsp_adc_context_t *context,
                                 hal_adc_handle_t *handle)
{
    if ((handle == NULL) || (HAL_ADC_Start(handle) != HAL_OK) ||
        (HAL_ADC_Calibrate(handle) != HAL_OK))
    {
        return false;
    }

    context->dma_sample = 0U;
    context->latest_raw = 0U;
    context->sample_sequence = 0U;
    context->dma_complete_count = 0U;
    context->error_count = 0U;
    context->initialized = true;
    context->busy = false;
    context->dma_complete_pending = false;
    context->dma_error_pending = false;
    context->stop_complete_pending = false;
    context->stop_requested = false;
    context->publish_after_stop = false;
    context->sample_ready = false;
    return true;
}

/* 测量完成后先请求 ADC 停下来，等收到“已经停止”的通知再交出读数，不能立即开始下次测量。 */
static void bsp_adc_process_context(bsp_adc_context_t *context,
                                    hal_adc_handle_t *handle)
{
    if (!context->initialized)
    {
        return;
    }

    if (!context->stop_requested &&
        (context->dma_complete_pending || context->dma_error_pending))
    {
        context->publish_after_stop =
            context->dma_complete_pending && !context->dma_error_pending;
        context->dma_complete_pending = false;
        context->dma_error_pending = false;
        context->stop_requested = true;
        if (HAL_ADC_REG_StopConv_DMA(handle) != HAL_OK)
        {
            context->stop_requested = false;
            context->publish_after_stop = false;
            context->busy = false;
            context->error_count++;
        }
    }

    if (context->stop_complete_pending)
    {
        context->stop_complete_pending = false;
        context->stop_requested = false;
        context->busy = false;
        if (context->publish_after_stop)
        {
            context->latest_raw = context->dma_sample;
            context->sample_sequence++;
            context->dma_complete_count++;
            context->sample_ready = true;
        }
        context->publish_after_stop = false;
    }
}

static bool bsp_adc_start_sample_context(bsp_adc_context_t *context,
                                         hal_adc_handle_t *handle)
{
    if (!context->initialized || context->busy || context->sample_ready)
    {
        return false;
    }

    context->busy = true;
    context->dma_sample = 0U;
    if (HAL_ADC_REG_StartConv_DMA_Opt(handle, (uint8_t *)&context->dma_sample,
                                      sizeof(context->dma_sample),
                                      HAL_ADC_OPT_DMA_IT_NONE) != HAL_OK)
    {
        context->busy = false;
        context->error_count++;
        return false;
    }
    return true;
}

static bool bsp_adc_read_sample_context(bsp_adc_context_t *context,
                                        uint32_t *raw,
                                        uint32_t *sequence)
{
    if ((raw == NULL) || (sequence == NULL) || !context->sample_ready)
    {
        return false;
    }

    *raw = context->latest_raw;
    *sequence = context->sample_sequence;
    context->sample_ready = false;
    return true;
}

static bool bsp_adc_get_status_context(const bsp_adc_context_t *context,
                                       bsp_adc_vbat_status_t *status)
{
    if ((status == NULL) || !context->initialized)
    {
        return false;
    }

    status->busy = context->busy;
    status->sample_ready = context->sample_ready;
    status->latest_raw = context->latest_raw;
    status->sample_sequence = context->sample_sequence;
    status->dma_complete_count = context->dma_complete_count;
    status->error_count = context->error_count;
    return true;
}

bool bsp_adc_vbat_init(void)
{
    return bsp_adc_init_context(&vbat_context, mx_adc1_gethandle());
}

void bsp_adc_vbat_process(void)
{
    bsp_adc_process_context(&vbat_context, mx_adc1_gethandle());
}

bool bsp_adc_vbat_start_sample(void)
{
    return bsp_adc_start_sample_context(&vbat_context, mx_adc1_gethandle());
}

bool bsp_adc_vbat_read_sample(uint32_t *raw, uint32_t *sequence)
{
    return bsp_adc_read_sample_context(&vbat_context, raw, sequence);
}

bool bsp_adc_vbat_get_status(bsp_adc_vbat_status_t *status)
{
    return bsp_adc_get_status_context(&vbat_context, status);
}

bool bsp_adc_key_init(void)
{
    return bsp_adc_init_context(&key_context, mx_adc2_gethandle());
}

void bsp_adc_key_process(void)
{
    bsp_adc_process_context(&key_context, mx_adc2_gethandle());
}

bool bsp_adc_key_start_sample(void)
{
    return bsp_adc_start_sample_context(&key_context, mx_adc2_gethandle());
}

bool bsp_adc_key_read_sample(uint32_t *raw, uint32_t *sequence)
{
    return bsp_adc_read_sample_context(&key_context, raw, sequence);
}

bool bsp_adc_key_get_status(bsp_adc_key_status_t *status)
{
    return bsp_adc_get_status_context(&key_context, status);
}

static bsp_adc_context_t *bsp_adc_context_from_handle(hal_adc_handle_t *handle)
{
    if (handle == mx_adc1_gethandle())
    {
        return &vbat_context;
    }
    if (handle == mx_adc2_gethandle())
    {
        return &key_context;
    }
    return NULL;
}

void HAL_ADC_REG_DataTransferCpltCallback(hal_adc_handle_t *handle)
{
    bsp_adc_context_t *context = bsp_adc_context_from_handle(handle);

    if (context != NULL)
    {
        context->dma_complete_pending = true;
    }
}

void HAL_ADC_REG_DataTransferStopCallback(hal_adc_handle_t *handle)
{
    bsp_adc_context_t *context = bsp_adc_context_from_handle(handle);

    if (context != NULL)
    {
        context->stop_complete_pending = true;
    }
}

void HAL_ADC_ErrorCallback(hal_adc_handle_t *handle)
{
    bsp_adc_context_t *context = bsp_adc_context_from_handle(handle);

    if (context != NULL)
    {
        context->dma_error_pending = true;
        context->error_count++;
    }
}
