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
	case POM_EV_BREAK_END:
		melody_start( JINGLE_BREAK_END );
		break;
	case POM_EV_LONG_BREAK_END:
		melody_start( JINGLE_LONG_BREAK_END );
		break;
	default:
		break;
	}
}

int main( void )
{
	pomodoro_t pom;
	uint32_t last_draw;

	SystemInit();
	funGpioInitAll();
	display_init();

	printf( "pacer: pomodoro timer\n" );

	pomodoro_init( &pom, funSysTick32(), (uint32_t)Ticks_from_Ms( STARTUP_TEST_MS ) );
	display_show( DISPLAY_NUM_SETS, DISPLAY_NUM_BARS, 1 );
	melody_start( JINGLE_STARTUP );
	last_draw = funSysTick32();

	while( 1 )
	{
		uint32_t now = funSysTick32();
		pom_event_t ev = pomodoro_update( &pom, now,
			(uint32_t)Ticks_from_Ms( FOCUS_MS ),
			(uint32_t)Ticks_from_Ms( BREAK_MS ),
			(uint32_t)Ticks_from_Ms( LONG_BREAK_MS ),
			(uint32_t)Ticks_from_Ms( STARTUP_TEST_MS ) );

		if( ev != POM_EV_NONE )
		{
			// Countdown of the new phase starts on this exact tick,
			// the same tick its jingle starts playing.
			play_for_event( ev );
		}

		melody_poll();

		if( ev != POM_EV_NONE
			|| TimeElapsed32( now, last_draw + DISPLAY_REFRESH_TICKS ) >= 0 )
		{
			if( pom.phase == PH_BOOT )
			{
				display_show( DISPLAY_NUM_SETS, DISPLAY_NUM_BARS, 1 );
			}
			else
			{
				uint32_t elapsed = now - pom.phase_start;
				display_show( pom.sets_completed,
					bars_for( elapsed, pom.phase_len ),
					pom_break_on( pom.phase ) );
			}
			last_draw = now;
		}
	}
}
