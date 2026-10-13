#ifndef _PACER_DISPLAY_H
#define _PACER_DISPLAY_H

// 74HC595 driver for the 3x daisy-chained shift registers + PC7 break LED.
//
// Bit map of the 24-bit word (1A -> 3H order):
//   bit0 = 1A = Set4, bit1 = 1B = Set3, bit2 = 1C = Set2, bit3 = 1D = Set1
//   bits4..23 = remaining-time bar, bit4 = rightmost (1E) .. bit23 = leftmost (3H)
// Shifted MSB-first: bit23 (3H) clocks out first, bit0 (1A) last.

#include <stdint.h>

#ifndef DISPLAY_HOST_TEST
#include "ch32fun.h"
#endif

#define DISPLAY_LATCH_PIN PC4
#define DISPLAY_BREAK_PIN PC7

#define DISPLAY_NUM_BARS 20
#define DISPLAY_NUM_SETS 4

static inline uint32_t display_word( uint8_t sets_completed, uint8_t bars_lit )
{
	uint32_t word = 0;

	if( sets_completed > DISPLAY_NUM_SETS )
	{
		sets_completed = DISPLAY_NUM_SETS;
	}
	if( bars_lit > DISPLAY_NUM_BARS )
	{
		bars_lit = DISPLAY_NUM_BARS;
	}

	// Sets light Set1-first: Set1 = bit3 ... Set4 = bit0.
	for( uint8_t i = 0; i < sets_completed; i++ )
	{
		word |= ( uint32_t )1 << ( 3 - i );
	}

	// Bar fills from the rightmost LED (bit4) toward the left.
	for( uint8_t i = 0; i < bars_lit; i++ )
	{
		word |= ( uint32_t )1 << ( 4 + i );
	}

	return word;
}

#ifndef DISPLAY_HOST_TEST

static inline void display_spi_init( void )
{
	SPI1->CTLR1 = 0;

	RCC->APB2PCENR |= RCC_APB2Periph_SPI1;

	// PC5 = SCLK, PC6 = MOSI (alt-function push-pull).
	funPinMode( PC5, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF );
	funPinMode( PC6, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF );

	SPI1->CTLR1 |= SPI_CPHA_1Edge | SPI_CPOL_Low
		| SPI_Mode_Master | SPI_BaudRatePrescaler_2
		| SPI_NSS_Soft | SPI_DataSize_8b;

	SPI1->CTLR1 |= SPI_Direction_2Lines_FullDuplex;

	SPI1->CTLR1 |= CTLR1_SPE_Set;
}

static inline void display_latch_pulse( void )
{
	funDigitalWrite( DISPLAY_LATCH_PIN, FUN_HIGH );
	funDigitalWrite( DISPLAY_LATCH_PIN, FUN_LOW );
}

static inline void display_send24( uint32_t word )
{
	// MSB first: bit23 (3H, far end of chain) goes out first.
	for( int8_t shift = 16; shift >= 0; shift -= 8 )
	{
		uint8_t byte = ( uint8_t )( ( word >> shift ) & 0xFF );
		while( !( SPI1->STATR & SPI_STATR_TXE ) ) {}
		SPI1->DATAR = byte;
		while( !( SPI1->STATR & SPI_STATR_RXNE ) ) {}
		( void )SPI1->DATAR;
	}
	while( SPI1->STATR & SPI_STATR_BSY ) {}
	display_latch_pulse();
}

static inline void display_init( void )
{
	funPinMode( DISPLAY_LATCH_PIN, GPIO_Speed_50MHz | GPIO_CNF_OUT_PP );
	funDigitalWrite( DISPLAY_LATCH_PIN, FUN_LOW );
	funPinMode( DISPLAY_BREAK_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );
	funDigitalWrite( DISPLAY_BREAK_PIN, FUN_LOW );
	display_spi_init();
}

static inline void display_show( uint8_t sets_completed, uint8_t bars_lit, uint8_t break_on )
{
	funDigitalWrite( DISPLAY_LATCH_PIN, FUN_LOW );
	display_send24( display_word( sets_completed, bars_lit ) );
	funDigitalWrite( DISPLAY_BREAK_PIN, break_on ? FUN_HIGH : FUN_LOW );
}

#endif /* DISPLAY_HOST_TEST */

#endif
