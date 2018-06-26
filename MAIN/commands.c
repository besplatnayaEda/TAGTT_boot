

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "jnx_common.h"
#include "jnx_console.h"
//#include "jnx_eeprom.h"
#include "tof.h"
//#include "uart_data.h"
//#include "lcd_driver.h"

// For Dump
ulong set_base_io( ulong base );
static ushort offs_io = 0;
static ulong  base_io = 0;


int mode_wb = 4;
int mode_io = 4;

#if( USE_TOF > 1 )

ushort tid = TAG_AIR_DEFAULT_ID;
ushort tof_mode = 0x0A;

#endif


#include <JPT.h>


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void dle_key( bint key )
{
	switch( key )
	{
#if( USE_TOF == 2 )
//	case 't':
//		tof_start( BEACON_DEFAULT_ADDR, 0 );
//
//		break;
#endif

/*
	case 'i':
		println( "Broadcast invitation" );
		send_bi();

		break;
*/

	case 'k':
		println( "DLE+k" );

		break;

	}

}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int commands( struct cmd_s *cmd )			// убрать лишние команды
{
int i;
int opsize;

	switch( cmd->cmdcode )
	{

//-------------------------------------
//#include "xall_commands.c"
//-------------------------------------


//-------------------------------------
case 'rlev':										// уровень сигнала

		{
		    vJPT_RadioDeInit();


		    bJPT_RadioInit( E_JPT_MODE_LOPOWER );


            BYTE energy = u8JPT_EnergyDetect( 16, 100 );

            xprintf( "LEV:-%2udB\r", i16JPT_ConvertEnergyTodBm( energy ) );

		    vJPT_RadioDeInit();


		}

		break;


//-------------------------------------
case 'bif ':							// список маяков?
		bif_list();
		break;


//--- Bootloader ------------------------
#if (DEV_TAGXX == 10)
// --- стереть память
	case 'cf  ':
		bAHI_FlashEraseSector(4);
		bAHI_FlashEraseSector(5);
		bAHI_FlashEraseSector(6);
		bAHI_FlashEraseSector(7);
		bAHI_FlashEraseSector(8);
		break;
// --- загрузить прошивку
	case 'ghex':
		hex_loader();
		break;

#endif
//--- Not Foud -----------------------
default:
cmd_error:
			println( "  <?>\a" );
			return( 1 );
	}
	return( 0 );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
ulong bcd2bin( ulong value )
{
ulong retv, mul;
	for( mul=1, retv=0; mul <= 10000000; mul *=10 )
	{   retv += (value&0x0F)*mul;
		value = (value >> 4);
	}

	return( retv ); 		// BCD to DEC

}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void dumpmem( ulong base, ushort offs, int n, int mode )
{
void *ptr;
uchar ch;
int   ngr, opsize;

	if( mode == 3 )
		opsize = 1;
	else
		opsize = mode;

	while( n > 0 )
	{
//#pragma diag_suppress=Pe181
		xprintf( "%8X+%2X\t", base, offs );
		for( ngr=(16/opsize); ngr; ngr-- )
		{   ptr = (void *)(base+offs);
			if( opsize == 4 )      	// 32bit
				xprintf( "%08X ", *(ulong *)ptr );
			else if( opsize == 2 ) 	// 16bit
				xprintf( "%04X ", *(ushort *)ptr );
			else 					//  8bit
			{	ch = *(BYTE *)ptr;
				xprintf( "%02X ", ch );
				if( mode == 3 )  		// Additional Ascii Dump
				{   if( ch < ' ' )
						ch = '.';
					xprintf( "%c", ch );
				}
			}
//#pragma diag_warning=Pe181
			offs += opsize;

			if( (n -=opsize) <= 0 )
				break;
		}

//		if( mode == 3 )			// Ascii Dump
//		{   while( tob_ptr < &tobuf[80-17] )
//				*( tob_ptr++) = ' ';
//			tobuf[asc] = '\0';
//		}

		xprintf( "\r" );
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void dumpstatus(void)
{
 	xprintf( "OP:");
	switch( mode_wb )
	{
	case 1:
		xprintf( " 8bit");
		break;
	case 2:
		xprintf( "16bit");
		break;
	case 3:
 		xprintf( "ASCII");
		break;
	case 4:
 		xprintf( "32bit");
		break;
	}
	xprintf( "  IO:" );
	switch( mode_io & 0x0007 )
	{
	case 1:
		xprintf( " 8bit");
		break;
	case 2:
		xprintf( "16bit");
		break;
	case 4:
 		xprintf( "32bit");
		break;
	default:
 		xprintf( "?");
	}
	xprintf( (mode_io>=0x80) ? "-Single\r" : "-Read+Write\r" );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
ulong set_base_io( ulong base )
{
	switch( base )
	{
	case 0xB0:
		base = 0x80000000;
		break;

	case 0xB1:
		base = 0x81000000;
		break;

	default:
			 ;
	}
	return( base );
}


