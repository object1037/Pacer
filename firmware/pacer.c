#include "ch32fun.h"
#include <stdio.h>

#include "config.h"
#include "display.h"
#include "melody.h"
#include "pomodoro.h"

#define DISPLAY_REFRESH_TICKS Ticks_from_Ms( 100 )

static inline void play_for_event( pom_event_t ev )
{
	switch( ev )
	{
	case POM_EV_FOCUS_END:
		melody_start( JINGLE_FOCUS_END );
		break;
	case POM_EV_FINAL_FOCUS_END:
		melody_start( JINGLE_LONG_BREAK_START );
		break;
	case POM_EV_BREAK_END:
		melody_start( JINGLE_BREAK_END );
		break;
	case POM_EV_LONG_BREAK_END:
		melody_start( JINGLE_CYCLE_START );
		break;
	default:
		break;
	}
}

int main( void )
{
	pomodoro_t pom;
	tickclock_t clock;
	uint32_t last_draw;
	uint8_t blink_toggle = 0;
	uint32_t blink_start_ms = pom_blink_idle( BLINK_MS );
	uint32_t last_blink_ms = 0;

	SystemInit();
	funGpioInitAll();
	display_init();

	printf( "pacer: pomodoro timer\n" );

	// All phase logic runs in MILLISECONDS (see tickclock_t): raw SysTick
	// deadlines can neither be computed (int overflow) nor stored (uint32
	// laps every ~24 min) for 25-30 min phases. Short-horizon users
	// (melody scheduler, display refresh) keep using raw ticks.
	tickclock_init( &clock, funSysTick32() );
	pomodoro_init( &pom, 0, STARTUP_TEST_MS );
	display_show( DISPLAY_NUM_SETS, DISPLAY_NUM_BARS, 1 );
	melody_start( JINGLE_CYCLE_START );
	last_draw = funSysTick32();

	while( 1 )
	{
		uint32_t now_tick = funSysTick32();
		uint32_t now_ms = tickclock_ms( &clock, now_tick, DELAY_MS_TIME );
		pom_event_t ev = pomodoro_update( &pom, now_ms,
			FOCUS_MS, BREAK_MS, LONG_BREAK_MS, STARTUP_TEST_MS );

		if( ev != POM_EV_NONE )
		{
			// Countdown of the new phase starts on this exact tick,
			// the same tick its jingle starts playing. The blink cue
			// window starts here too (except at startup), independent
			// of jingle length.
			play_for_event( ev );
			if( pom_starts_blink( ev ) )
			{
				blink_start_ms = now_ms;
				blink_toggle = 1;
				last_blink_ms = now_ms;
			}
		}

		melody_poll();

		if( ev != POM_EV_NONE
			|| TimeElapsed32( now_tick, last_draw + DISPLAY_REFRESH_TICKS ) >= 0 )
		{
			uint8_t blink_on = pom_blink_active( now_ms, blink_start_ms, BLINK_MS );

			if( blink_on && pom.phase != PH_BOOT
				&& (uint32_t)( now_ms - last_blink_ms ) >= BLINK_TOGGLE_MS )
			{
				blink_toggle ^= 1;
				last_blink_ms = now_ms;
			}
			if( pom.phase == PH_BOOT )
			{
				display_show( DISPLAY_NUM_SETS, DISPLAY_NUM_BARS, 1 );
			}
			else
			{
				uint32_t elapsed = now_ms - pom.phase_start;
				display_show( pom.sets_completed,
					bars_for( elapsed, pom.phase_len ),
					pom_pc7_out( pom.phase, blink_on, blink_toggle ) );
			}
			last_draw = now_tick;
		}
	}
}
