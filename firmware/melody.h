#ifndef _PACER_MELODY_H
#define _PACER_MELODY_H

// Non-blocking jingle player on top of tone.h (TIM2).
//
// This is the fix for tone_demo.c's blocking pattern (tone + Delay_Ms
// between notes stalls everything): melody_start() fires the first note
// immediately and returns, melody_poll() advances notes from the main
// loop, so the phase countdown keeps ticking while the jingle plays.
//
// All jingles share one uniform total duration so remaining-time
// accounting is identical no matter which jingle just played:
//   150 + 30(gap) + 150 + 30(gap) + 300 = 660 ms.

#include <stdint.h>

#ifdef MELODY_HOST_TEST
// ---- host simulation stubs (real declarations live in the test) ----
#define MELODY_BUZZER_PIN 0
#define Ticks_from_Ms( n ) ( n )
#define TimeElapsed32( now, start ) ( (int32_t)( (uint32_t)( now ) - (uint32_t)( start ) ) )
uint32_t funSysTick32( void );
void host_tone_stub( uint32_t pin, unsigned int freq, unsigned long dur_ms );
#define MELODY_TONE( pin, freq, dur ) host_tone_stub( ( pin ), ( freq ), ( dur ) )
#define MELODY_NOTONE( pin ) ( ( void )( pin ) )
#else
#include "ch32fun.h"
#define TONE_IMPLEMENTATION
#include "tone.h"
#define MELODY_BUZZER_PIN PD4
#define MELODY_TONE( pin, freq, dur ) tone( ( pin ), ( freq ), ( dur ) )
#define MELODY_NOTONE( pin ) noTone( pin )
#endif

typedef enum
{
	JINGLE_FOCUS_END,
	JINGLE_BREAK_END,
	JINGLE_LONG_BREAK_START,
	JINGLE_CYCLE_START,
	JINGLE_COUNT
} jingle_t;

typedef struct
{
	uint16_t freq_hz;
	uint16_t dur_ms;
} melody_note_t;

#define MELODY_NOTES_PER_JINGLE 5
#define MELODY_GAP_MS 10
// Uniform total, verified by the host sim: (150+10)*4 + 300.
#define MELODY_TOTAL_MS 940

static const melody_note_t melody_table[ JINGLE_COUNT ][ MELODY_NOTES_PER_JINGLE ] = {
	// FOCUS_END
	{{659, 150}, {831, 150}, {988, 150}, {831, 150}, {1319, 300}},
	// BREAK_END
	{{880, 150}, {880, 150}, {988, 150}, {988, 150}, {1047, 300}},
	// LONG_BREAK_END
	{{1047, 150}, {1047, 100}, {0, 200}, {1175, 150}, {1319, 300}},
	// STARTUP
	{{880, 150}, {880, 100}, {0, 200}, {1109, 150}, {1319, 300}},
};

static uint8_t melody_busy_flag;
static uint8_t melody_note_idx;
static jingle_t melody_current;
static uint32_t melody_next_tick;

static inline void melody_start( jingle_t j )
{
	if( (uint8_t)j >= (uint8_t)JINGLE_COUNT )
	{
		return;
	}
	melody_current = j;
	melody_note_idx = 0;
	melody_busy_flag = 1;
	MELODY_TONE( MELODY_BUZZER_PIN,
		melody_table[ j ][ 0 ].freq_hz, melody_table[ j ][ 0 ].dur_ms );
	melody_next_tick = funSysTick32()
		+ (uint32_t)Ticks_from_Ms( melody_table[ j ][ 0 ].dur_ms + MELODY_GAP_MS );
}

static inline void melody_poll( void )
{
	uint8_t last;

	if( !melody_busy_flag )
	{
		return;
	}
	if( TimeElapsed32( funSysTick32(), melody_next_tick ) < 0 )
	{
		return;
	}
	melody_note_idx++;
	if( melody_note_idx >= MELODY_NOTES_PER_JINGLE )
	{
		melody_busy_flag = 0;
		return;
	}
	last = ( melody_note_idx == MELODY_NOTES_PER_JINGLE - 1 );
	MELODY_TONE( MELODY_BUZZER_PIN,
		melody_table[ melody_current ][ melody_note_idx ].freq_hz,
		melody_table[ melody_current ][ melody_note_idx ].dur_ms );
	melody_next_tick = funSysTick32() + (uint32_t)Ticks_from_Ms(
		melody_table[ melody_current ][ melody_note_idx ].dur_ms
		+ ( last ? 0 : MELODY_GAP_MS ) );
}

static inline uint8_t melody_busy( void )
{
	return melody_busy_flag;
}

#endif
