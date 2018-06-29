

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "jnx_common.h"
#include "jnx_console.h"

#ifndef USE_DATA_UART
#define USE_DATA_UART  (0)
#endif

#if( USE_DATA_UART )
#include "uart_data.h"
#endif


#if( (TBUF_SIZE & TBUF_SIZE_MSK) != 0 )
	#error TBUF_SIZE must be a power of 2.
#endif

//void boutchar( uchar  ch );
static void outchar_buf( uchar  ch );

//---------------------------------------------------------------------------
// HEX Conversion table
const char HexChars[16] = "0123456789ABCDEF";


struct txd_buf
{   ushort tail;       		// Next In Index
  	ushort head;   			// Next Out Index
  	char buf[TBUF_SIZE];   	// Buffer body
	bint   busy_flag;
};

struct cmd_buf
{   ushort tail;       		// Input size
//  	ushort dummy;
	char buf[CLIN_SIZE+4];  // Buffer body. +2 for "  /r/0"
};

// Systems FLAGS
volatile ulong sys_status_word;
// = STW_TXACKNAK;
// Output buffer
static struct txd_buf tbuf; 		// Pointers to beginn
// Comandline buffer
static struct cmd_buf cbuf;

// Comandline parse result
struct cmd_s  xcmd;


int cbuf_parser( cmd_s *cmd, char *lin_ptr );


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void disable_console( bint uart )
{
	vAHI_UartDisable( uart );
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void init_console( bint uart, ulong baud )
{
#if( USE_CONSOLE )

	// RTS/CTS not used 2-wire mode
	vAHI_UartSetRTSCTS( uart, FALSE );
    // Enable UART
    vAHI_UartEnable( uart );
    // Reset UART
    vAHI_UartReset( uart, TRUE, TRUE  );
    vAHI_UartReset( uart, FALSE, FALSE);

    // Register function that will handle UART interrupts
	if( uart == E_AHI_UART_0 )
	     vAHI_Uart0RegisterCallback( console_isr );
	else
         vAHI_Uart1RegisterCallback( console_isr );

    // Set the clock divisor register to give required baud, this has to be done
    //   directly as the normal routines (in ROM) do not support all baud rates
    uart_setbaudrate( uart, baud );

    // Set remaining settings
    vAHI_UartSetControl( uart, FALSE, FALSE, E_AHI_UART_WORD_LEN_8, TRUE, FALSE );


#if( USE_DATA_UART )
 	#define CONSOLE_INT_PRIORITY ( UART_DATA_INT_PRIORITY )
	#define CONSOLE_INT_FIFO 	 ( UART_DATA_FIFO_LEVEL )
#else
 	#define CONSOLE_INT_PRIORITY (1)
	#define CONSOLE_INT_FIFO 	 ( E_AHI_UART_FIFO_LEVEL_14 )
#endif
	vAHI_InterruptSetPriority( MICRO_ISR_MASK_UART0, CONSOLE_INT_PRIORITY );


    // Turn on modem status, tx, rx interrupts
    vAHI_UartSetInterrupt( uart, FALSE, FALSE, TRUE, TRUE, CONSOLE_INT_FIFO  );


/*
#if( USE_DATA_UART || DEV_ISPTX )
	// Пороверяем наличие подключенного терминала
	vAHI_UartWriteData( CONSOLE_UART, ENQ );
	for( volatile int i=0; i< 500000; i++ )
	{   if( sw_chk( STW_TERMINAL ) )
			break;
	}
#else
	// Безусловно присутствует терминал
	sw_set( STW_TERMINAL );
#endif
*/


#else
ARGSUSED( uart );
ARGSUSED( baud );
#endif
}




//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void cmd_parser(void)
{
	cbuf_parser( &xcmd, &cbuf.buf[0] );		// Call Command Line parser

	cbuf.tail = 0; 							// Pionter to start
	outchar( XON );							// Run terminal
}



//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int cbuf_parser( cmd_s *cmd, char *ptr )
{
char *cmd_ptr;
char ch;
//int  i;

	cmd->command_line_orig = ptr;

	if( *ptr == '\0' )
		return( 0 ); 			// Empty string

	while( *ptr <= ' ' ) 	// Skip leaders 'spaces'
		ptr++;

	cmd_ptr = ptr;

	while( *ptr > ' ' ) 	// Skip command name
		ptr++;

	if( ptr - cmd_ptr < 4 )
	{	// Add spaces to command name if its to short
		cmd->cmdcode = '    '; 		// 4 spaces
		memcpy( &cmd->cmdcode, cmd_ptr, ptr - cmd_ptr );
	}
	else
		memcpy( &cmd->cmdcode, cmd_ptr, 4 );

	cmd->argc = 0;                  // Clear arguments counter
	cmd->argn[ARGSS] = 0xFFFFFFFF;	// Clear 'segment'

	while( *ptr <= ' ' )		// Skip delimiter
	{	if( *ptr == '\0' )
		{	cmd->command_line = ptr;
			goto end_parser;
		}
		ptr++;
	}
	cmdupr( ptr );
	cmd->command_line = ptr;

	// Поиск аргументов
	for( ; ; )
	{
		ch = *ptr++;
		if( (ch == 'N')||(ch == '@')||(ch == 'R')||(ch == 'W')||(ch == 'K' )||(ch == 'S' )||(ch == 'L' )||(ch == 'I' )||(ch == 'P' )||(ch == 'T' ) )
		{   cmd->argc++;
			cmd->argn[cmd->argc] = ch;
		}
		else
		{	if( isxdigit( (int)ch ) )
			{   // Hex Digit - Argument found
				for( ulong value = 0; ; )
				{
					if( ch >= 'A' )
						value += (ch-'A'+10 );
					else
						value += (ch-'0');

					// Get and Check next
					ch = *ptr++;
					if( ch == ':' )
					{	// Разделитель двоеточие
						cmd->argn[ARGSS] = value;
						cmd->argc = 0;
						// Find next argument
   						break;
					}
					if( !isxdigit( (int)ch ) )
					{	// End Argument - delimiter, or end string
						cmd->argc++;
						cmd->argn[cmd->argc] = value;
						if( ch == '\0' )
							goto end_parser;
						if( cmd->argc >= ARGNN )
							goto end_parser;

						break;
					}
					else
						value <<= 4;
				}
			}
			else if( ch == ':' )
			{	// Разделитель двоеточие
				cmd->argn[ARGSS] = cmd->argn[cmd->argc];
  				cmd->argc = 0;
			}
			else if( ch == '\0' )
				goto end_parser;
		}
	}

end_parser:
/*
	if( sw_chk( STW_ESC_KEY ) )
	{   sw_clr( STW_ESC_KEY );
		println( "ESC Pressed...\r");
	}
*/
	return( commands( cmd ) );
}


/*
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int putchar( int ch )
{
	if(	status_word & STW_THR_BUSY )
	{	// Всегда в буфер если FIFO UART не пустое
  		tbuf.buf[tbuf.tail & TBUF_SIZE_MSK] = ch;
  			tbuf.tail++;
    }
	else
	{ 	status_word |= STW_THR_BUSY;
		vAHI_UartWriteData( CONSOLE_UART, ch );
	}
	return( ch );
}
*/



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void outchar( uchar ch )
{
#if( USE_CONSOLE )
	if(	tbuf.busy_flag )
	{	// Всегда в буфер если FIFO UART не пустое
  		tbuf.buf[tbuf.tail & TBUF_SIZE_MSK] = ch;
  			tbuf.tail++;
    }
	else
	{ 	tbuf.busy_flag = TRUE;
		vAHI_UartWriteData( CONSOLE_UART, ch );
	}
#else
	ARGSUSED( ch );
#endif
}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void outchar_buf( uchar ch )
{
// Всегда в буфер
	tbuf.buf[tbuf.tail & TBUF_SIZE_MSK] = ch;
  		tbuf.tail++;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void println( char *string )
{
#if( USE_CONSOLE )
	if(	tbuf.busy_flag == FALSE )
	{   tbuf.busy_flag  = TRUE;
		vAHI_UartWriteData( CONSOLE_UART, *string++ );
	}
	while( *string )
  	{	tbuf.buf[tbuf.tail & TBUF_SIZE_MSK] = *string++;
  			tbuf.tail++;
	}
  	tbuf.buf[tbuf.tail & TBUF_SIZE_MSK] = '\r';
  			tbuf.tail++;
#else
	ARGSUSED( string );
#endif
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void printst( char *string )
{
#if( USE_CONSOLE )
	if(	tbuf.busy_flag == FALSE )
	{   tbuf.busy_flag  = TRUE;
		vAHI_UartWriteData( CONSOLE_UART, *string++ );
	}
	while( *string )
  	{	tbuf.buf[tbuf.tail & TBUF_SIZE_MSK] = *string++;
  			tbuf.tail++;
	}
#else
	ARGSUSED( string );
#endif
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void console_isr( ulong dev, ulong bitmap )
{
BYTE ch;
    if( dev == CONSOLE_UART_DEVICE )
    { 	// Data to receive ?
        if( bitmap & E_AHI_UART_INT_RXDATA )
        {  	// Receive character from UART
			ch = u8AHI_UartReadData( CONSOLE_UART );

#if( USE_DATA_UART )
			if( sw_chk( STW_DATA_FRAME ) )
			{   if( ud.iwr < MAX_GAS_DATA_FRAME )
					ud.data[ud.iwr++] = ch;
				uart_frame_timeout_restart();
				goto continue_isr;
			}
#endif

			if( u8AHI_UartReadLineStatus( CONSOLE_UART ) &( E_AHI_UART_LS_ERROR|E_AHI_UART_LS_FE|E_AHI_UART_LS_OE|E_AHI_UART_LS_BI ) )
			{

			}
			else
			{
			if( !sw_chk( STW_HEX_LOADER ) )
			{
			switch( ch )
			{

#if( USE_DATA_UART )

			case 0x00:

				if( ud.state != UD_STATE_OFF )
				{
					led_on( BIT_LED_GREEN_MAIN );
					sw_set( STW_DATA_FRAME );
					ud.iwr = 0;
					uart_frame_timeout_start();
				}
				break;

			case UART_DATA_REQUEST:
				// Выкидывать эхо запроса данных и взводить флаг валидности предачи запроса
				if( chk_df_state( UD_STATE_TX_WAIT ) )
					set_ud_state( UD_STATE_TX_REQUEST_DONE );

				break;

			case UART_DATA_LIVE:
				// Выкидывать эхо и ничего не делать
				break;
#endif

			case '\r':

				outchar( ch );					// Echo
				outchar_buf( XOFF );			// Stop terminal transmit request
				cbuf.buf[cbuf.tail] = '\0';		// EOL
				sw_set_isr( STW_COMMAND );		// Parser request

				break;

			case '\b':
				if( cbuf.tail > 0 )			// If Backspace
				{	--(cbuf.tail);
					outchar( ch );			// Back cursor
					outchar_buf( ' ' );	// Clear
					outchar_buf( ch );	    // Back cursor
				}
				break;

			case ACK:
				sw_set( STW_TERMINAL );

				break;

#ifdef STW_DLE_KEY
			case DLE:
				sw_set( STW_DLE_KEY );
				break;
#endif
#if DEV_TAGXX == 10
			case 'H':
				if( sw_chk( STW_DLE_KEY ) )
				{
					print_byte( ACK );
					sw_set( STW_HEX_LOADER );
					//hex_loader();
				}
				break;
#endif
			case ESC:
				sw_set( STW_ESC_KEY );
				break;

			default:

				if( cbuf.tail < CLIN_SIZE )				// Continue input
				{

#ifdef STW_DLE_KEY
					if( sw_chk( STW_DLE_KEY ) )
					{   sw_clr( STW_DLE_KEY );
						dle_key( ch );
					}
					else
#endif
					{	outchar( ch ); 				// Echo
       					cbuf.buf[(cbuf.tail)++] = ch;  	//	and Store
					}
				}

			}	//	switch( ch )
			}	// 	if( sw_chk( STW_HEX_LOADER ) )
			else
			{
				hex_loader();
			}

			}
        }

#if( USE_DATA_UART )
continue_isr:
#endif
        // Ready to transmit ?
        if( bitmap & E_AHI_UART_INT_TX )
		{  	if( tbuf.tail != tbuf.head )
			{   for( int i=0; ( tbuf.tail != tbuf.head )&&( i < TR_FIFO_SIZE ); i++ )
 					vAHI_UartWriteData( CONSOLE_UART, tbuf.buf[(tbuf.head++) & TBUF_SIZE_MSK] );
			}
//			else
//				tbuf.busy_flag  = FALSE;

			if( tbuf.tail == tbuf.head )
				tbuf.busy_flag  = FALSE;
		}
    }
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void cmdupr( char *ptr )
{
	for( ; *ptr &&( *ptr != '/' ); ptr++ )
		*ptr = toupper( (int)*ptr );
}



//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void xprintf( const char *format, ... )
{
#if( USE_CONSOLE )

bint format_flag;
uchar ch;
ulong u_val, t_val, temp;
bint format_size = 4;
va_list ap;

	va_start( ap, format );


	for( ; ; )
	{   while( (format_flag = *format++) != '%' )	// Until '%' or '\0'
		{	if( !format_flag )                      //
			{   va_end( ap );
				return;
			}
			outchar( format_flag );
		}

		format_flag = *( (char *)format++ );
		switch( format_flag )
		{
	case 's':
			printst( va_arg( ap, char * ) );
			break;
	case 'c':
// 		   	format_flag = va_arg( ap, char );
 		   	format_flag = va_arg( ap, int );
	default:
			outchar( format_flag );
			continue;

	case '0':	// Дополнение 0 игнорировать, ибо всегда и без этого так
			format_flag = *( (char *)format++ );
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	    	format_size = format_flag - '0';
			format_flag = *( (char *)format++ );
			if( format_flag == 'u' )
				goto dec_value;
	case 'X':
			u_val = va_arg( ap, ulong );
			switch( format_size )
			{
		case 8:
				outchar( hexchar(u_val >> 28) );
		case 7:
				outchar( hexchar(u_val >> 24) );
		case 6:
				outchar( hexchar(u_val >> 20) );
		case 5:
				outchar( hexchar(u_val >> 16) );
		case 4:
				outchar( hexchar(u_val >> 12) );
		case 3:
				outchar( hexchar(u_val >> 8 ) );
		case 2:
				outchar( hexchar(u_val >> 4 ) );
		case 1:
				outchar( hexchar(u_val      ) );
			}
			break;
dec_value:
	case 'u':
			u_val = va_arg( ap, ulong );
			t_val = u_val;
			switch( format_size )
			{
		case 8:
				temp = 10000000; ch	= '0';
				while( t_val >= temp )
				{	ch++;
					t_val -= temp;
				};
				outchar( ch );
		case 7:
				temp = 1000000;	 ch	= '0';
				while( t_val >= temp )
				{	ch++;
					t_val -= temp;
				};
				outchar( ch );
		case 6:
				temp = 100000;	 ch	= '0';
				while( t_val >= temp )
				{	ch++;
					t_val -= temp;
				};
				outchar( ch );
		case 5:
				temp = 10000;	 ch	= '0';
				while( t_val >= temp )
				{	ch++;
					t_val -= temp;
				};
				outchar( ch );
		case 4:
				temp = 1000;	 ch	= '0';
				while( t_val >= temp )
				{	ch++;
					t_val -= temp;
				};
				outchar( ch );
		case 3:
				temp = 100;		 ch	= '0';
				while( t_val >= temp )
				{	ch++;
					t_val -= temp;
				};
				outchar( ch );
		case 2:
				temp = 10;		 ch	= '0';
				while( t_val >= temp )
				{	ch++;
					t_val -= temp;
				};
				outchar( ch );
		case 1:
				outchar( '0' + t_val );
			}
			break;
		}
	}

#else
ARGSUSED( format );
#endif
}

//---------------------------------------------------------------------------
// Print 8-bit Hex value
//---------------------------------------------------------------------------
void print_byte_sp( BYTE data  )
{
	outchar( hexchar(data >> 4) );
	outchar_buf( hexchar(data ) );
	outchar_buf( ' ' );
}

//---------------------------------------------------------------------------
// Print 8-bit Hex value
//---------------------------------------------------------------------------
void print_byte( BYTE data  )
{
	outchar( hexchar(data >> 4) );
	outchar_buf( hexchar(data ) );
}

//---------------------------------------------------------------------------
// Print 16-bit Hex value
//---------------------------------------------------------------------------
void print_word_sp( ushort data  )
{
	// Print 16-bit hex value
	outchar( hexchar(data >> 12) );
	outchar_buf( hexchar(data >> 8 ) );
	outchar_buf( hexchar(data >> 4 ) );
	outchar_buf( hexchar(data) );
	outchar_buf( ' ' );
}

//---------------------------------------------------------------------------
// Print 16-bit Hex value
//---------------------------------------------------------------------------
void print_word( ushort data  )
{
	// Print 16-bit hex value
	outchar(     hexchar( data >> 12 ) );
	outchar_buf( hexchar( data >> 8  ) );
	outchar_buf( hexchar( data >> 4  ) );
	outchar_buf( hexchar( data       ) );
}

//---------------------------------------------------------------------------
// Print 32-bit Hex value
//---------------------------------------------------------------------------
void print_long( ulong data  )
{
	// Print 32-bit hex value
	outchar( hexchar(     data >> 28 ) );
	outchar_buf( hexchar( data >> 24 ) );
	outchar_buf( hexchar( data >> 20 ) );
	outchar_buf( hexchar( data >> 16 ) );
	outchar_buf( hexchar( data >> 12 ) );
	outchar_buf( hexchar( data >> 8  ) );
	outchar_buf( hexchar( data >> 4  ) );
	outchar_buf( hexchar( data       ) );
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void print_buff_sp( BYTE *buff, bint size )
{
	if( size == 0 )
		return;

	outchar(     hexchar( *buff >> 4 ) );
	outchar_buf( hexchar( *buff++    ) );
	outchar_buf( ' ' );

	while( --size )
	{	outchar_buf( hexchar( *buff >> 4 ) );
		outchar_buf( hexchar( *buff++    ) );
		outchar_buf( ' ' );
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void print_buff( BYTE *buff, bint size )
{
	if( size == 0 )
		return;

	outchar( hexchar(     *buff >> 4 ) );
	outchar_buf( hexchar( *buff++    ) );

	while( --size )
	{	outchar_buf( hexchar( *buff >> 4 ) );
		outchar_buf( hexchar( *buff++    ) );
	}
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint print_check(void)
{

//	if( tbuf.tail == tbuf.head )
//		return( FALSE );
//	return( TRUE );

	return( tbuf.busy_flag );
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void print_all(void)
{
	while( tbuf.tail != tbuf.head )
	{ 	while( ( u8AHI_UartReadLineStatus( CONSOLE_UART ) & E_AHI_UART_LS_TEMT ) == 0  );

		vAHI_UartWriteData( CONSOLE_UART, tbuf.buf[(tbuf.head++) & TBUF_SIZE_MSK] );
	}
	while( ( u8AHI_UartReadLineStatus( CONSOLE_UART ) & E_AHI_UART_LS_TEMT ) == 0  );

}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void print_wait_all(void)
{
	while( tbuf.busy_flag );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int uart_setbaudrate( bint uart, ulong baud )
{
 //	cfg.baud = baud;

ushort divisor = 0;
ulong  remain;
bint   clocks_per_bit = 16;

    /* Defining ENABLE_ADVANCED_BAUD_SELECTION in the Makefile
     * enables this code which searches for a clocks per bit setting
     * that gets closest to the configured rate.
     */
ulong calc_baud = 0;
ulong baud_error = 0x7FFFFFFF;

    while( abs( baud_error ) > (baud >> 4)) // 6.25% (100/16) error
    {
        if( --clocks_per_bit < 3 )
            return( 1 );
        // Calculate Divisor register = 16MHz / (16 x baud rate)
        divisor = (ushort)(SYS_FREQUENCY /( (clocks_per_bit+1) * baud ) );

        // Correct for rounding errors
        remain = (ulong)(SYS_FREQUENCY % ((clocks_per_bit+1) * baud ) );

        if( remain >= ((( clocks_per_bit+1) * baud) / 2 ) )
            divisor++;

        calc_baud = (SYS_FREQUENCY / ((clocks_per_bit+1) * divisor ) );
        baud_error = calc_baud - baud;
    }
    // Set the calculated clocks per bit
    vAHI_UartSetClocksPerBit( uart, clocks_per_bit );
    // Set the calculated divisor
    vAHI_UartSetBaudDivisor( uart, divisor );
	return( 0 );
}


