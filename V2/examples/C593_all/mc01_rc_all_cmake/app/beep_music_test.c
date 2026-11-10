#include "beep_music_test.h"

#include <stdbool.h>
#include <stddef.h>

#include "bsp_beep.h"

#define BEEP_NOTE_REST           (0U)
#define BEEP_NOTE_GS7            (3322U)
#define BEEP_NOTE_B7             (3951U)
#define BEEP_NOTE_D8             (4699U)
#define BEEP_NOTE_DS8            (4978U)
#define BEEP_NOTE_GAP_MS         (15U)

typedef struct
{
    uint16_t frequency_hz;
    uint16_t duration_ms;
} beep_music_note_t;

typedef struct
{
    uint32_t phase_start_time_ms;
    bool note_active;
} beep_music_test_context_t;

/* Monophonic approximation of the Spidey Tracker opening jingle. */
static const beep_music_note_t spidey_tracker_jingle[] =
{
    {BEEP_NOTE_GS7, 330U},
    {BEEP_NOTE_B7, 170U},
    {BEEP_NOTE_DS8, 210U},
    {BEEP_NOTE_REST, 450U},
    {BEEP_NOTE_D8, 350U},
    {BEEP_NOTE_B7, 170U},
    {BEEP_NOTE_GS7, 310U}
};

static volatile beep_music_test_summary_t beep_music_test_status;
static beep_music_test_context_t beep_music_test_context;

static bool beep_music_test_start_note(uint32_t current_time_ms)
{
    const beep_music_note_t *note = &spidey_tracker_jingle[beep_music_test_status.note_index];

    if ((note->frequency_hz != BEEP_NOTE_REST) &&
        (!bsp_beep_set_frequency(note->frequency_hz) || !bsp_beep_start()))
    {
        return false;
    }

    beep_music_test_context.phase_start_time_ms = current_time_ms;
    beep_music_test_context.note_active = true;
    beep_music_test_status.current_frequency_hz = note->frequency_hz;
    return true;
}

static void beep_music_test_fail(void)
{
    (void)bsp_beep_stop();
    beep_music_test_context.note_active = false;
    beep_music_test_status.current_frequency_hz = 0U;
    beep_music_test_status.result = BEEP_MUSIC_TEST_FAILED;
}

void beep_music_test_init(uint32_t current_time_ms)
{
    beep_music_test_status.result = BEEP_MUSIC_TEST_RUNNING;
    beep_music_test_status.note_index = 0U;
    beep_music_test_status.note_count =
        sizeof(spidey_tracker_jingle) / sizeof(spidey_tracker_jingle[0]);
    beep_music_test_status.current_frequency_hz = 0U;
    beep_music_test_context.note_active = false;
    beep_music_test_context.phase_start_time_ms = current_time_ms;

    if (!bsp_beep_init() || !beep_music_test_start_note(current_time_ms))
    {
        beep_music_test_fail();
    }
}

void beep_music_test_process(uint32_t current_time_ms)
{
    const beep_music_note_t *note;

    if (beep_music_test_status.result != BEEP_MUSIC_TEST_RUNNING)
    {
        return;
    }

    note = &spidey_tracker_jingle[beep_music_test_status.note_index];
    /* 分别记住何时开始发声、何时开始间隔；没到时间就先返回，让主循环继续处理其他演示。 */
    if (beep_music_test_context.note_active)
    {
        if ((current_time_ms - beep_music_test_context.phase_start_time_ms) <
            (note->duration_ms - BEEP_NOTE_GAP_MS))
        {
            return;
        }

        if ((note->frequency_hz != BEEP_NOTE_REST) && !bsp_beep_stop())
        {
            beep_music_test_fail();
            return;
        }
        beep_music_test_context.note_active = false;
        beep_music_test_context.phase_start_time_ms = current_time_ms;
        beep_music_test_status.current_frequency_hz = 0U;
        return;
    }

    if ((current_time_ms - beep_music_test_context.phase_start_time_ms) < BEEP_NOTE_GAP_MS)
    {
        return;
    }

    beep_music_test_status.note_index++;
    if (beep_music_test_status.note_index >= beep_music_test_status.note_count)
    {
        beep_music_test_status.result = BEEP_MUSIC_TEST_PASSED;
        return;
    }

    if (!beep_music_test_start_note(current_time_ms))
    {
        beep_music_test_fail();
    }
}

bool beep_music_test_get_summary(beep_music_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    *summary = beep_music_test_status;
    return true;
}
