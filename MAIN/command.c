

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "jnx_common.h"
#include "jnx_console.h"
//#include "jnx_eeprom.h"
#include "tof.h"
#include "uart.h"

// For Dump
ulong set_base_io( ulong base );
static ushort offs_io = 0;
static ulong  base_io = 0;


int mode_wb = 4;
int mode_io = 4;
ushort tid = TAG_AIR_DEFAULT_ID;

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
int command( struct cmd_s *cmd )			// убрать лишние команды
{
int i;
int opsize;

	switch( cmd->cmdcode )
	{

#if( IOCH_NN )
//-------------------------------------
case 'iots':
		iod.ch[0].i_scan = FALSE;
		iod.ch[1].i_scan = FALSE;
		{
		ulong state_x;
		pin_hi( OUT_1|OUT_2 );
		dummy_delay( 2000 );
	   	state_x = u32AHI_DioReadInput();
		if( (state_x &( KEY_1|KEY_2 ) ) != ( KEY_1|KEY_2 ) )
			xprintf( "Bad inital state: %8X\r", state_x &( KEY_1|KEY_2 ) );

		pin_lo( OUT_1 );
		dummy_delay( 2000 );
	   	state_x = u32AHI_DioReadInput();
		if( state_x & KEY_1 )
			xprintf( "CH1 On Fail\r" );
		pin_hi( OUT_1 );
		dummy_delay( 2000 );
	   	state_x = u32AHI_DioReadInput();
		if( ( state_x & KEY_1 ) == 0 )
			xprintf( "CH1 Off Fail\r" );


		pin_lo( OUT_2 );
		dummy_delay( 2000 );
	   	state_x = u32AHI_DioReadInput();
		if( state_x & KEY_2 )
			xprintf( "CH2 On Fail\r" );
		pin_hi( OUT_2 );
		dummy_delay( 2000 );
	   	state_x = u32AHI_DioReadInput();
		if( ( state_x & KEY_2 ) == 0 )
			xprintf( "CH2 Off Fail\r" );

		}
  //		cfg_parser();

 		println( "Done" );

		break;

//-------------------------------------
case 'utst':

		uart_tx_on();
		uart_rx_on();

		vAHI_UartWriteData( RS485_UART, 0x55 );
		dummy_delay( 22000 );
		vAHI_UartWriteData( RS485_UART, 0xAA );
		dummy_delay( 22000 );

		uart_rx_off();
		vAHI_UartWriteData( RS485_UART, 0x02 );
		dummy_delay( 22000 );

		uart_rx_on();
		uart_tx_off();
		vAHI_UartWriteData( RS485_UART, 0x03 );
		dummy_delay( 22000 );

		break;


#endif

//-------------------------------------
case 's   ':
			if( cmd->argc  != 1 )
				goto cmd_error;
			if( cfg.airid.id  == TAG_AIR_DEFAULT_ID  )
			{	cfg_set_air_id( bcd2bin( cmd->argn[1] ) );
				break;
			}
			goto cmd_error;
//-------------------------------------
case 'cfg ':
case 'set ':

		switch( cmd->argc  )
		{
		case 0:
  	 		cfg_read();
			cfg_dump();

			break;

		case 1:
			switch( cmd->argn[ARGSS] )
			{
			case 'W':
				if( eeprom_cfg_store() == 0 )
					println( "Ok" );
				else
					println( "Failed" );
				break;

			case 'I':
				if( cfg_set_air_id( bcd2bin( cmd->argn[1] ) ) )
					goto cmd_error;
				break;

			case 0xC:
				cfg.airch.ch = (BYTE)bcd2bin( cmd->argn[1] );
//				eeprom_cfg_store();
				break;

			case 0xF:
				cfg.airch.features = (BYTE)bcd2bin( cmd->argn[1] );
//				eeprom_cfg_store();
				break;

			case 'P':
				cfg.airch.out_power = cmd->argn[1];
//				eeprom_cfg_store();
				break;


			case 'L':
				 if( cfg_set_config_full( (tag_config_u)cmd->argn[1] ) )
					goto cmd_error;
				break;

			}
			break;

		default:
			goto cmd_error;

		}
		break;

//-------------------------------------
#if( USE_TOF == 2 )
case 'tof ':
		switch( cmd->argc  )
		{

  		case 0:
			tof_uni( tid );
			break;

		case 1:

			tid = bcd2bin( cmd->argn[1] );
			tof_start( tid, 0 );
			break;

		case 2:
			tid = bcd2bin( cmd->argn[1] );
			tof_start( tid, cmd->argn[2] );
			break;

		default:
			goto cmd_error;

		}
		break;
#endif
//-------------------------------------
case 'rese':
		if( cmd->argc == 1 )
		{
			if( cmd->argn[1] == 0x4321 )
			{
				println( "Full Reprogramm..." );
				flash_invalidate();
				printall();
				reset( 0 );
			}
			else
				reset( cmd->argn[1] );
		}
		else
			reset( 0 );

		break;

/*
//-------------------------------------
case 'txdt':
		if( cmd->argc != 1 )
			goto cmd_error;
		if( cmd->argn[1] )
			status_word |= STW_TXDT;
		else
			status_word &= (~STW_TXDT);

		break;
*/



//-------------------------------------
case 'txd ':
		if( cmd->argc < 1 )
			goto cmd_error;

		if( cmd->argc < 2 )
			cmd->argn[2] = 1;

		while( cmd->argn[2]-- )
		{
//			ulong timeout = 0;
//			status_word &= (~STW_TXACKNAK);
			tx_data_packet( cmd->argn[1], (BYTE *)"Hello!", 7 );

//			while( (status_word & STW_TXACKNAK) == 0 )
//			{  	if( timeout++ > 100000 )
//				{   println( "TXD:Timeout" );
//				 	status_word |= STW_TXACKNAK;
//					break;
//				}
//			}

		}

		break;

//-------------------------------------
case 'txdx':
		if( cmd->argc < 2 )
			goto cmd_error;

		tx_data_packet( cmd->argn[1], (BYTE *)"Hello!", 7 );

		break;

//-------------------------------------
case 'ps  ':

		println( "Ok" );
		break;


/*
//-------------------------------------
case 'scan':
		set_state( STATE_IDLE );

		start_active_scan();

		break;

case 'asso':
		start_associate();
		break;

*/


/*
//---  input output ------------------
case 's   ':
case 'io  ':
		{
		ulong plong = 0;
		ulong mbit;
		BYTE *ptr;
		ulong *ptr_ul;

		if( !(cmd->argc) )
			goto cmd_error;

		if( cmd->argn[ARGSS] != 0xFFFFFFFF )
			base_io = set_base_io( cmd->argn[ARGSS] );
		if( mode_wb == 3 )
			opsize = 1;
		else
			opsize = mode_wb;

		ptr = (BYTE *)( base_io + cmd->argn[1] );
		xprintf( "%8X+%2X\t", base_io, cmd->argn[1] );

		if( ( mode_io < 0x80 )||( cmd->argc != 3 ) )
		{

			if( base_io >= 0xE0000000 )
			{
				ptr_ul = (void *)ptr;
				plong =  *(ulong *)ptr_ul;

				if( (mode_io & 0x07) == 4 )			// 32bit display
					xprintf( "%8X", plong );
				else if( (mode_io & 0x07) == 2 )	// 16bit display
					xprintf( "%4X", plong & 0xFFFF );
		    	else                    			//  8bit display
					xprintf( "%02X", plong & 0x00FF );
			}
			else
			{   if( opsize == 4 )					// 32bit
					xprintf( "%8X", plong = *(ulong *)ptr );
				else if( opsize == 2 )				// 16bit
					xprintf( "%4X", plong = *(ushort *)ptr );
		    	else                    			//  8bit
					xprintf( "%2X", plong = *(BYTE *)ptr );
		    }
		}
		switch( cmd->argc )
		{
	case 2: 								// Write port
			plong = cmd->argn[2];
			goto wrandp;
	case 3: 		                		// BitSet Mode
			mbit = bcd2bin( cmd->argn[2] );
		  	mbit = 1<<mbit;

			switch( cmd->argn[3] )
		  	{
		case 0:           	        // Set to 0
				plong &= ~mbit;
				break;
		case 1:                     // Set to 1
				plong |= mbit;
				break;
		case 3:                		// Reverse BIT
				plong ^= mbit;
				break;
		default:
				goto cmd_error;
			}
	 wrandp:
			if( base_io >= 0xE0000000 )
			{   *(ulong  *)ptr = plong;

				if( (mode_io & 0x07) == 4 )			// 32bit display
					xprintf( "<-%8X", plong );
				else if( (mode_io & 0x07) == 2 )	// 16bit display
					xprintf( "<-%4X", plong & 0xFFFF);
		    	else                    			//  8bit display
					xprintf( "<-%2X", plong & 0x00FF );
			}
			else
			{   if( opsize == 4 )			// 32bit
				{	*(ulong  *)ptr = plong;
					xprintf( "<-%8X", cmd->argn[2] );
				}
				else if( opsize == 2 )		// 16bit
				{	*(ushort *)ptr = (ushort)plong;
					xprintf( "<-%4X", plong );
				}
				else                    	//  8bit
				{	*(BYTE   *)ptr = (BYTE)plong;
					xprintf( "<-%02lX", plong );
				}
			}
	case 1:      	                    // Only Read Port - print and done
			xprintf( "\r" );
			break;
	default:
			goto cmd_error;
		}

		}
	break;
*/

/*
//--- enter ---------------------------
case 'e   ':
		if( cmd->argc < 2 )
			goto cmd_error;

		if( cmd->argn[ARGSS] != 0xFFFFFFFF )
			base_io = set_base_io( cmd->argn[ARGSS] );
		if( base_io >= 0xE0000000 )
			mode_wb	= 4;
		if( mode_wb == 3 )
			opsize = 1;
		else
			opsize = mode_wb;

		{
		BYTE *ptr = (BYTE *)(base_io+cmd->argn[1]);

		for( i=1; i< cmd->argc ; i++ )
		{   if( opsize == 4 )		// 32bit
				*(ulong   *)ptr = (ulong)cmd->argn[1+i];
			else if( opsize == 2 ) 	// 16bit
				*(ushort *)ptr = (ushort)cmd->argn[1+i];
			else                  	//  8bit
				*(BYTE   *)ptr =   (BYTE)cmd->argn[1+i];
			ptr += opsize;
		}
		}
		break;
*/

//--- set bit ---------------------------
case 'sb  ':
		if( cmd->argc != 3 )
			goto cmd_error;

		if( cmd->argn[ARGSS] != 0xFFFFFFFF )
			base_io = set_base_io( cmd->argn[ARGSS] );
		{
		ulong vlong, mbit;
		ulong *ptr = (ulong *)(base_io+cmd->argn[1]);
		mbit = bcd2bin( cmd->argn[2] );
	  	mbit = 1<<mbit;
		vlong = *ptr;

		switch( cmd->argn[3] )
	  	{
	case 0:           	        // Set to 0
			vlong &= ~mbit;
			break;
	case 1:                     // Set to 1
			vlong |= mbit;
			break;
	case 3:                		// Reverse BIT
			vlong ^= mbit;
			break;
	default:
			goto cmd_error;
		}
	  	*ptr = vlong;
		}
		break;

//--- d ump ---------------------------
case 'd   ':
			i = 16;
			switch( cmd->argc )
			{
		case 2:
				i = cmd->argn[2]-cmd->argn[1]+1;
				if( i < 0  )
					i = cmd->argn[2];
				if( (i<=0)||(i>0xB0) )
					goto cmd_error;
		case 1:
				offs_io = cmd->argn[1];
		case 0:
				break;
		default:
				goto cmd_error;
			}

		if( cmd->argn[ARGSS] != 0xFFFFFFFF )
			base_io = set_base_io( cmd->argn[ARGSS] );
		if( base_io >= 0xE0000000 )
			mode_wb	= 4;
		if( mode_wb == 3 )
			opsize = 1;
		else
			opsize = mode_wb;

		dumpmem( base_io, offs_io, i, mode_wb );
		offs_io += opsize;

		break;


//--- mode ----------------------------
case 'mode':
			if( cmd->argc )
			{	switch( cmd->argn[1] )
				{
		case 1:
		case 2:
		case 3:
		case 4:
					mode_wb = cmd->argn[1];
					break;
		case 'I':
					if( cmd->argc == 2 )
						mode_io = (mode_io & 0x0080)|(cmd->argn[2] & 0x0007);
					else
						mode_io ^= 0x80;
					break;
		default:
					goto cmd_error;

				}
			}
			dumpstatus();

			break;

//-------------------------------------
	case 'debu':
			switch( cmd->argc )
			{
		case 1:
				cfg.debug = cmd->argn[1];
		case 0:
				xprintf( "SDL:%8X\r", cfg.debug );
				break;
		case 2:
				if( cmd->argn[2] )
					cfg.debug |=  ( 1 << bcd2bin(cmd->argn[1]) );
				else
					cfg.debug &= ~( 1 << bcd2bin(cmd->argn[1]) );

				xprintf( "SDL:%8X\r", cfg.debug );
				break;

		default:
				goto cmd_error;
			}
			break;

//-------------------------------------
	case 'stw ':
			switch( cmd->argc )
			{
		case 0:
				xprintf( "SDL:%8X\r", sw_get() );
				break;
		case 2:
				if( cmd->argn[2] )
				{
					sw_set( 1 << bcd2bin(cmd->argn[1] ) );
				}
				else
				{
					sw_clr( 1 << bcd2bin(cmd->argn[1] ) );
				}

				xprintf( "SDL:%8X\r", sw_get() );
				break;

		default:
				goto cmd_error;
			}
			break;

//-------------------------------------
	case 'ps ':
		println( "Ok" );
		break;


//--- Dummy arguments print -----------
case 'aa  ':
		xprintf( "\nargc=%1u ls=[%s]\r", cmd->argc, cmd->command_line );
		xprintf( "argn[S]=%8lX\r", cmd->argn[0] );
		for( i=1; i<= cmd->argc; i++ )
			xprintf( "argn[%1u]=%8lX\r", i, cmd->argn[i] );
		break;

//--- check ---------------------------
case 'ch  ':
		if( cmd->argc < 2 )
			goto cmd_error;
	 	switch( cmd->argn[1] )
		{

	//-----------
	case 'R':		// RAM
		println( "RAM Test. Wait..." );
		break;

	//------------
	default:
		println( "No Object" );
		goto cmd_error;

		}
		break;

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


