#ifndef _PACER_POMODORO_H
#define _PACER_POMODORO_H

// Pure Pomodoro cycle logic — no MCU dependencies, unit-agnostic.
// The caller passes `now` and phase lengths in ONE consistent time unit:
// firmware passes SysTick ticks (lengths converted with Ticks_from_Ms),
// the host test passes milliseconds directly.

#include <stdint.h>

#define POM_BARS 20

typedef enum
{
	PH_BOOT,
	PH_FOCUS,
	PH_BREAK,
	PH_LONG_BREAK
} phase_t;

typedef enum
{
	POM_EV_NONE,
	POM_EV_BOOT_DONE, // BOOT self-test over; focus countdown starts now
	POM_EV_FOCUS_END, // play JINGLE_FOCUS_END, countdown of next phase starts now
	POM_EV_BREAK_END, // play JINGLE_BREAK_END, countdown of next phase starts now
	POM_EV_LONG_BREAK_END // play JINGLE_LONG_BREAK_END, cycle resets now
} pom_event_t;

typedef struct
{
	phase_t phase;
	uint8_t set_number; // 1..4, the focus set we are in / entering
	uint8_t sets_completed; // 0..4, finished focuses so far
	uint32_t phase_start; // time unit of the caller
	uint32_t phase_len; // length of the current phase, same unit
} pomodoro_t;

// Remaining-time bar: starts full, extinguishes right-to-left (ceil so the
// last LED only goes out at expiry).
static inline uint8_t bars_for( uint32_t elapsed, uint32_t total )
{
	uint64_t remaining;

	if( total == 0 || elapsed >= total )
	{
		return 0;
	}
	remaining = (uint64_t)( total - elapsed ) * POM_BARS;
	return (uint8_t)( ( remaining + ( total - 1 ) ) / total );
}

static inline void pomodoro_init( pomodoro_t *p, uint32_t now, uint32_t boot_len )
{
	p->phase = PH_BOOT;
	p->set_number = 1;
	p->sets_completed = 0;
	p->phase_start = now;
	p->phase_len = boot_len;
}

// Advance the machine; returns the event for `now` (at most one per call).
// The caller starts the matching jingle AND the next countdown on this tick.
static inline pom_event_t pomodoro_update( pomodoro_t *p, uint32_t now,
	uint32_t focus_len, uint32_t break_len, uint32_t long_break_len, uint32_t boot_len )
{
	( void )boot_len; // consumed once by pomodoro_init(); kept for call symmetry
	if( (uint32_t)( now - p->phase_start ) < p->phase_len )
	{
		return POM_EV_NONE;
	}

	switch( p->phase )
	{
	case PH_BOOT:
		p->phase = PH_FOCUS;
		p->set_number = 1;
		p->sets_completed = 0;
		p->phase_start = now;
		p->phase_len = focus_len;
		return POM_EV_BOOT_DONE;

	case PH_FOCUS:
		p->phase_start = now;
		// The focus just finished: its set counts as completed immediately,
		// so the count is visible during the following break / long break.
		p->sets_completed = p->set_number;
		if( p->set_number < 4 )
		{
			p->phase = PH_BREAK;
			p->phase_len = break_len;
			return POM_EV_FOCUS_END;
		}
		p->phase = PH_LONG_BREAK;
		p->phase_len = long_break_len;
		return POM_EV_FOCUS_END;

	case PH_BREAK:
		p->set_number++;
		p->phase = PH_FOCUS;
		p->phase_start = now;
		p->phase_len = focus_len;
		return POM_EV_BREAK_END;

	case PH_LONG_BREAK:
		p->sets_completed = 0;
		p->set_number = 1;
		p->phase = PH_FOCUS;
		p->phase_start = now;
		p->phase_len = focus_len;
		return POM_EV_LONG_BREAK_END;
	}

	return POM_EV_NONE;
}

// PC7 break indicator: HIGH only in BOOT (lamp test) / BREAK / LONG_BREAK.
static inline uint8_t pom_break_on( phase_t phase )
{
	return ( phase == PH_FOCUS ) ? 0 : 1;
}

// PC7 output with jingle visual cue: while the post-phase-change blink
// window is active (outside BOOT) the LED blinks under the caller's toggle
// bit instead of following the break rule, so each phase change has a
// visual cue even in FOCUS. BOOT stays solid so the lamp test shows every
// LED lit.
static inline uint8_t pom_pc7_out( phase_t phase, uint8_t blink_active, uint8_t blink_toggle )
{
	if( blink_active && phase != PH_BOOT )
	{
		return blink_toggle ? 1 : 0;
	}
	return pom_break_on( phase );
}

// Blink window check: true for `len` time units after `start` (wrap-safe).
// The window is independent of jingle length.
static inline uint8_t pom_blink_active( uint32_t now, uint32_t start, uint32_t len )
{
	return (uint32_t)( now - start ) < len;
}

// Whether this phase-machine event opens a blink window: only the
// end-of-phase events do. BOOT_DONE (startup) never blinks.
static inline uint8_t pom_starts_blink( pom_event_t ev )
{
	return ev == POM_EV_FOCUS_END
		|| ev == POM_EV_BREAK_END
		|| ev == POM_EV_LONG_BREAK_END;
}

#endif
