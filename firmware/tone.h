/* Arduino-compatible tone() for piezo buzzers and other square-wave audio.

   Uses TIM2 update interrupts to toggle a GPIO pin at the requested
   frequency. Works on any digital output pin (no timer AF mapping required).

   Usage in one source file:
	#define TONE_IMPLEMENTATION
	#include "tone.h"

   API (matches Arduino):
	tone( pin, frequency );                 // play until noTone()
	tone( pin, frequency, duration_ms );    // play for duration_ms
	noTone( pin );

   tone() uses TIM2. Do not use TIM2 for other purposes while a tone is active.

   Copyright 2026, under the MIT License.
*/

#ifndef _TONE_H
#define _TONE_H

#include <stdint.h>

void tone_impl( uint32_t pin, unsigned int frequency, unsigned long duration_ms );
void noTone( uint32_t pin );

#define tone_continuous( pin, frequency ) tone_impl( (pin), (frequency), 0 )

/* GCC extension: tone( pin, freq ) plays until noTone(). */
#define tone( pin, frequency, ... ) tone_impl( pin, frequency, ##__VA_ARGS__ + 0 )

#ifdef TONE_IMPLEMENTATION

#include "ch32fun.h"
#include "funconfig.h"

#ifndef TONE_TIMER
#define TONE_TIMER TIM2
#endif

#ifndef TONE_TIMER_IRQn
#define TONE_TIMER_IRQn TIM2_IRQn
#endif

#ifndef TONE_TIMER_RCC_APB1
#define TONE_TIMER_RCC_APB1 RCC_APB1Periph_TIM2
#endif

#ifndef TONE_TIM_CLK
#define TONE_TIM_CLK FUNCONF_SYSTEM_CORE_CLOCK
#endif

static volatile uint32_t tone_pin;
static volatile uint8_t tone_active;
static volatile uint8_t tone_pin_high;
static volatile uint32_t tone_end_tick;

static void tone_gpio_enable( uint32_t pin )
{
#if defined( CH32H41x )
	RCC->HB2PCENR |= RCC_HB2Periph_GPIOA << ( (pin) >> 4 );
#elif !defined( CH5xx )
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA << ( (pin) >> 4 );
#else
	( void )pin;
#endif
}

static void tone_stop( void )
{
	TONE_TIMER->CTLR1 &= (uint16_t)~TIM_CEN;
	TONE_TIMER->DMAINTENR &= (uint16_t)~TIM_IT_Update;
	NVIC_DisableIRQ( TONE_TIMER_IRQn );

	if( tone_active )
	{
		funDigitalWrite( tone_pin, FUN_LOW );
		tone_active = 0;
		tone_pin_high = 0;
	}
}

static void tone_configure_timer( unsigned int frequency )
{
	uint32_t half_period = (uint32_t)TONE_TIM_CLK / (uint32_t)frequency / 2U;
	uint16_t prescaler = 1;

	while( ( half_period / prescaler ) > 0xffffU )
	{
		prescaler++;
	}

	uint16_t period = (uint16_t)( half_period / prescaler );
	if( period < 1 )
	{
		period = 1;
	}

	RCC->APB1PCENR |= TONE_TIMER_RCC_APB1;
	RCC->APB1PRSTR |= TONE_TIMER_RCC_APB1;
	RCC->APB1PRSTR &= (uint32_t)~TONE_TIMER_RCC_APB1;

	TONE_TIMER->CTLR1 &= (uint16_t)~TIM_CEN;
	TONE_TIMER->PSC = (uint16_t)( prescaler - 1U );
	TONE_TIMER->ATRLR = (uint16_t)( period - 1U );
	TONE_TIMER->CNT = 0;
	TONE_TIMER->SWEVGR |= TIM_UG;
	TONE_TIMER->INTFR = (uint16_t)~TIM_FLAG_Update;
	TONE_TIMER->DMAINTENR = TIM_IT_Update;
	TONE_TIMER->CTLR1 |= TIM_CEN;
}

static void tone_timer_isr( void )
{
	if( TONE_TIMER->INTFR & TIM_FLAG_Update )
	{
		TONE_TIMER->INTFR = (uint16_t)~TIM_FLAG_Update;

		if( !tone_active )
		{
			return;
		}

		tone_pin_high ^= 1;
		funDigitalWrite( tone_pin, tone_pin_high ? FUN_HIGH : FUN_LOW );

		if( tone_end_tick != 0 && TimeElapsed32( funSysTick32(), tone_end_tick ) >= 0 )
		{
			tone_stop();
		}
	}
}

void noTone( uint32_t pin )
{
	if( !tone_active || pin != tone_pin )
	{
		return;
	}

	tone_stop();
}

void tone_impl( uint32_t pin, unsigned int frequency, unsigned long duration_ms )
{
	if( frequency == 0 )
	{
		noTone( pin );
		return;
	}

	if( tone_active )
	{
		if( pin != tone_pin )
		{
			funDigitalWrite( tone_pin, FUN_LOW );
		}
		tone_stop();
	}

	tone_gpio_enable( pin );
#if defined( CH32H41x )
	funPinMode( pin, GPIO_Mode_Out_PP, GPIO_Speed_50MHz );
#else
	funPinMode( pin, FUN_OUTPUT );
#endif

	tone_pin = pin;
	tone_pin_high = 0;
	funDigitalWrite( pin, FUN_LOW );

	if( duration_ms != 0 )
	{
		tone_end_tick = funSysTick32() + (uint32_t)duration_ms * DELAY_MS_TIME;
	}
	else
	{
		tone_end_tick = 0;
	}

	tone_configure_timer( frequency );
	tone_active = 1;
	NVIC_EnableIRQ( TONE_TIMER_IRQn );
}

#if !defined( CH5xx )
void TIM2_IRQHandler( void ) INTERRUPT_DECORATOR;
void TIM2_IRQHandler( void )
{
	tone_timer_isr();
}
#endif

#endif /* TONE_IMPLEMENTATION */

#endif /* _TONE_H */
