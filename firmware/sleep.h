#ifndef _PACER_SLEEP_H
#define _PACER_SLEEP_H

// Power saving without behavior change: the main loop is polling (every
// wake re-checks phase expiry, melody schedule, and refresh timers), so
// the core can sleep in WFI between periodic SysTick wakeups instead of
// busy-spinning. Pattern follows ch32fun's examples/systick_irq: CMP
// re-armed in the ISR, no auto-reload, so the free-running counter that
// Delay_Ms/funSysTick32 rely on is untouched. Worst case a wake is one
// SLEEP_WAKE_MS late — unobservable on minute-scale phases.

#include <stdint.h>

#ifndef DISPLAY_HOST_TEST
#include "ch32fun.h"
#endif

#ifndef SLEEP_WAKE_MS
#define SLEEP_WAKE_MS 10
#endif

#ifndef DISPLAY_HOST_TEST

static inline void sleep_init( void )
{
	SysTick->CMP = SysTick->CNT + (uint32_t)Ticks_from_Ms( SLEEP_WAKE_MS );
	SysTick->CTLR |= SYSTICK_CTLR_STIE;
	NVIC_EnableIRQ( SysTick_IRQn );
}

void SysTick_Handler( void ) __attribute__( ( interrupt ) );
void SysTick_Handler( void )
{
	SysTick->CMP += (uint32_t)Ticks_from_Ms( SLEEP_WAKE_MS );
	SysTick->SR = 0;
}

#endif /* DISPLAY_HOST_TEST */

#endif
