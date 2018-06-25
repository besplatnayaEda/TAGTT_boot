

#ifndef __CONSOLE_COMMS_H
#define __CONSOLE_COMMS_H

#include "jnx_common.h"


void init_console( bint uart, ulong baud );
void disable_console( bint uart );
void console_isr( ulong dev, ulong bitmap );
int uart_setbaudrate( bint uart, ulong baud );

#define TR_FIFO_SIZE	(14)
// Define which of the two available hardware UARTs and what baud rate to use
#define CONSOLE_UART          E_AHI_UART_0			// UART to use
#define CONSOLE_UART_DEVICE   E_AHI_DEVICE_UART0   	// UART device
#define CONSOLE_BAUD_RATE      (115200UL)

#define TBUF_SIZE 	(2048)
#define TBUF_SIZE_MSK   (TBUF_SIZE-1)
#define CLIN_SIZE 	(80)    		// Input command size



void cmd_parser(void);
void dle_key( bint key );

void cmdupr( char *ptr );
void println( char *string );
void xprintf( const char *format, ... );
void printst( char *string );
ulong bcd2bin( ulong value );
void dumpmem( ulong base, ushort offs, int n, int mode );
void dumpstatus(void);

void outchar( uchar  ch );
void outchar_isr( uchar  ch );
void print_byte( BYTE data  );
void print_byte_sp( BYTE data  );
void print_word( ushort data  );
void print_word_sp( ushort data  );
void print_long( ulong data  );
void print_buff( BYTE *buff, bint size );
void print_buff_sp( BYTE *buff, bint size );
void print_all(void);
void print_wait_all(void);
bint print_check(void);



#define ARGNN	16 			// Max Arguments
#define ARGSS	0			// ARGNN

extern const char HexChars[16];
#define hexchar(x)	HexChars[(x)&0x0F]

//---------------------------------------------------------------------------
typedef struct cmd_s{         		// Command
unsigned long cmdcode;
unsigned long argn[ARGNN+1]; // +1 for 0 ARG = segment
char *command_line_orig;
char *command_line;
bint argc;
} cmd_s;

bint commands( struct cmd_s *cmd );

#define print_ch( ch ) outchar( ch )


//---------------------------------------------------------------------------
#define 	SOH		0x01
#define 	STX 	0x02
#define 	ETX     0x03
#define 	EOT 	0x04
#define 	ENQ		0x05
#define 	ACK 	0x06
#define 	BEL		0x07
#define 	BELL	BEL
#define 	BS		0x08
#define 	TAB     0x09
#define 	LF      0x0A
#define 	VT      0x0B
#define 	FF      0x0C
#define 	CR      0x0D
#define 	SO      0x0E
#define 	SI_COD  0x0F       // ^O
#define 	DLE		0x10	   // Data Link Escape
#define 	XON		0x11       // or DC1
#define 	DC2	   	0x12	   // Device Control2
#define 	XOFF   	0x13       // or DC3
#define 	DC4     0x14
#define 	NAK		0x15	   // Negative Acknowledge
#define 	SYN		0x16	   // Synchronus Idle
#define 	ETB     0x17
#define 	CAN 	0x18
#define 	EM      0x19
#define 	SUB_COD 0x1A	   // Time Stamp
#define 	ESC		0x1B
//#define 	ESCCOD	ESC
#define 	FS_COD  0x1C
#define 	DGS		0x1D	   // Group Separator - Start of packet
#define 	RS_COD	0x1E
#define 	US_COD	0x1F 	   // ^_




#endif
