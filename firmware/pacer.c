#include "ch32fun.h"
#include <stdio.h>

#include "display.h"
#include "melody.h"

int main( void )
{
	SystemInit();
	funGpioInitAll();
	display_init();

	printf( "pacer melody check\n" );
	melody_start( JINGLE_STARTUP );

	while( 1 )
	{
		melody_poll();
		display_show( 0, DISPLAY_NUM_BARS, 0 );
		Delay_Ms( 10 );
		if( !melody_busy() )
		{
			display_show( 4, 0, 1 );
			Delay_Ms( 500 );
			melody_start( JINGLE_FOCUS_END );
		}
	}
}
