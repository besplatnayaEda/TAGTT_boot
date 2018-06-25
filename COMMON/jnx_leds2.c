
#include <stdlib.h>

#include "jnx_common.h"
#include "jnx_leds2.h"

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
static const ulong led_const[LM_MAX] =
					{ 0x00000000,			// 0 Off
				      0x11111111,			// 1 On
					  0x11110000,			// 2 Slow Blink
					  0x11001100,			// 3 Normal Blink
					  0x10101010,			// 4 Fast Blink
					  0x10000000,			// 5 Single Flash
					  0x10100000,			// 6 Two    Flash
					  0x10101000,			// 7 Tree   Flash
					  0x01111111,			// 8 Single Off
					  0x01011111			// 9 Two    Off
					};


led_data_s ldat;


#if( DEV_ISIB3 ||( DEV_TAGXX == 3 ) || DEV_ISBRF || DEV_BRF24 || DEV_IVT24 )
//---------------------------------------------------------------------------
//  2 LED processor
//---------------------------------------------------------------------------
void led_process( bint counter )
{
ulong curstat;
ulong curtemp;

	if( ldat.stat  == 0 )
	{	// ¬се всегда выключены. Ќичего не переписываем - оставл€ем возможность пр€мого управлени€
		// светодиодами
		return;

	}
	curtemp = (ldat.stat >> ((counter & 0x07 )<<2));
	curstat =  (( curtemp & (1<<NUM_LED_RED  ) )<<(POS_LED_RED  -NUM_LED_RED   )); 	// раз Led
	curstat |= (( curtemp & (1<<NUM_LED_GREEN) )<<(POS_LED_GREEN-NUM_LED_GREEN )); 	// два Led

	                                               						// 3..4 Not Used

	if( curstat & BIT_LED_DO_RED )
		pin_do_hi( BIT_LED_DO_RED );
	if( curstat & BIT_LED_GREEN )
		pin_hi( BIT_LED_GREEN );

	curstat = (~curstat)&(BIT_LED_DO_RED|BIT_LED_GREEN);

	if( curstat & BIT_LED_DO_RED )
		pin_do_lo( BIT_LED_DO_RED );
	if( curstat & BIT_LED_GREEN )
		pin_lo( BIT_LED_GREEN );


}
#elif( DEV_TAGXX )
//---------------------------------------------------------------------------
//  2 LED processor
//---------------------------------------------------------------------------
void led_process( bint counter )
{
ulong curstat;
ulong curtemp = (ldat.stat >> ((counter & 0x07 )<<2));

	curstat =  (( curtemp & (1<<NUM_LED_RED  ) )<<(POS_LED_RED  -NUM_LED_RED   )); 	// раз Led
	curstat |= (( curtemp & (1<<NUM_LED_GREEN_MAIN) )<<(POS_LED_GREEN_MAIN-NUM_LED_GREEN_MAIN )); 	// два Led

#if( DEV_TAGXX == 2 )
	if( curstat )
		pin_hi( curstat );
	curstat = (~curstat)&(BIT_LED_RED|BIT_LED_GREEN);
	if( curstat )
		pin_lo( curstat );
#elif( ( DEV_TAGXX == 4 )||( DEV_TAGXX == 6 )||( DEV_TAGXX == 7 )||( DEV_TAGXX == 9 ) )
	if( curstat )
		pin_hi( curstat );
	curstat = (~curstat)&(BIT_LED_RED|BIT_LED_GREEN_MAIN);
	if( curstat )
		pin_lo( curstat );

#elif( ( DEV_TAGXX == 3 )||( DEV_TAGXX == 5 ) )
	if( curstat & BIT_LED_DO_RED )
		pin_do_hi( BIT_LED_DO_RED );
	if( curstat & BIT_LED_GREEN )
		pin_hi( BIT_LED_GREEN );

	curstat = (~curstat)&(BIT_LED_DO_RED|BIT_LED_GREEN);

	if( curstat & BIT_LED_DO_RED )
		pin_do_lo( BIT_LED_DO_RED );
	if( curstat & BIT_LED_GREEN )
		pin_lo( BIT_LED_GREEN );
#else
#error "No Led Type"
#endif



}
#endif


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void led_mode( bint led, bint mode )
{
	if( led >= MAX_LEDS )
		return;

	if( mode >= LM_MAX )
		mode = LM_MAX-1;
	ldat.stat &= ((~0x11111111)  <<led);
	ldat.stat |= (led_const[mode]<<led);
	ldat.mode[led] = mode;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int led_mode_get( bint led )
{
	if( led >= MAX_LEDS )
		return( 0 );
	return( ldat.mode[led] );
}
