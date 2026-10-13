#include "ch32fun.h"
#include <stdio.h>

#include "display.h"

int main( void )
{
	SystemInit();
	funGpioInitAll();
	display_init();

	printf( "pacer display check\n" );

	while( 1 )
	{
		display_show( 0, DISPLAY_NUM_BARS, 0 );
		Delay_Ms( 500 );
		display_show( 4, 0, 1 );
		Delay_Ms( 500 );
	}
}
