/*
 * 让板载蜂鸣器播放一次旋律，运行后请听是否正常发声。
 * init 启动播放，主循环反复调用 process(HAL_GetTick()) 切换音符，不会停在这里等整段音乐播完。
 * PASSED 只说明程序完成了播放步骤，并没有麦克风判断声音；仍需人工听一下。
 */
#ifndef BEEP_MUSIC_TEST_H
#define BEEP_MUSIC_TEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BEEP_MUSIC_TEST_IDLE = 0U,
    BEEP_MUSIC_TEST_RUNNING,
    BEEP_MUSIC_TEST_PASSED,
    BEEP_MUSIC_TEST_FAILED
} beep_music_test_result_t;

typedef struct
{
    uint32_t result;
    uint32_t note_index;
    uint32_t note_count;
    uint32_t current_frequency_hz;
} beep_music_test_summary_t;

void beep_music_test_init(uint32_t current_time_ms);
void beep_music_test_process(uint32_t current_time_ms);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool beep_music_test_get_summary(beep_music_test_summary_t *summary);

#endif
