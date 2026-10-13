#include "ch32fun.h"

int main( void )
{
	SystemInit();
	funGpioInitAll();

	funPinMode( PC7, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );

	while( 1 )
	{
		funDigitalWrite( PC7, FUN_HIGH );
		Delay_Ms( 500 );
		funDigitalWrite( PC7, FUN_LOW );
		Delay_Ms( 500 );
	}
}
