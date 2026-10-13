#ifndef _PACER_CONFIG_H
#define _PACER_CONFIG_H

// Phase lengths. No durations are hardcoded in pacer.c — everything
// comes from these defines.
//
// Debug defaults (quick check, seconds):
#define FOCUS_MS 25000
#define BREAK_MS 5000
#define LONG_BREAK_MS 30000
#define STARTUP_TEST_MS 3000

// Production values (Pomodoro Technique, minutes):
// #define FOCUS_MS (25 * 60 * 1000)
// #define BREAK_MS (5 * 60 * 1000)
// #define LONG_BREAK_MS (30 * 60 * 1000)
// #define STARTUP_TEST_MS 3000

#endif
