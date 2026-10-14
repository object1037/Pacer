#ifndef _PACER_CONFIG_H
#define _PACER_CONFIG_H

// Phase lengths. No durations are hardcoded in pacer.c — everything
// comes from these defines.
//
// Debug defaults (quick check, seconds):
// #define FOCUS_MS 15000
// #define BREAK_MS 5000
// #define LONG_BREAK_MS 15000
// #define STARTUP_TEST_MS 3000

// Production values (Pomodoro Technique, minutes):
#define FOCUS_MS (25 * 60 * 1000)
#define BREAK_MS (5 * 60 * 1000)
#define LONG_BREAK_MS (30 * 60 * 1000)
#define STARTUP_TEST_MS 3000

// Break-LED blink cue after each phase change: window length (independent
// of jingle length) and blink toggle interval. Same for debug/production.
#define BLINK_MS 10000
#define BLINK_TOGGLE_MS 150

#define SLEEP_WAKE_MS 100
#define SLEEP_ENABLED 1

#endif
