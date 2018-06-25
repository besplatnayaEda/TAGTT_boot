
#ifndef __COMMON_JNX_DEF_H
#define __COMMON_JNX_DEF_H

#include <stdlib.h>
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <stdint.h>
#include <time.h>
#include <MicroInt.h>

#include "config.h"
#include "jnx_system.h"
#include "jnx_app_buff.h"
#include "jnx_console.h"
#include "xtag_protocol.h"

//#include "uart_protocol.h"

//#include "database.h"

//#if( DEV_ISBRF )
//#include "uart_ext.h"
//#endif


#if( DEV_TAGXX || DEV_ISPTX )
#include "cfg_tagx.h"
#endif

#if( DEV_ISIB3 || DEV_ISBRF || DEV_BRF24 )
#include "cfg_isibx.h"
#define USE_DATA_UART	(0)
#endif

#if( DEV_IVT24 )
#include "cfg_ivt24.h"
#define USE_DATA_UART	(0)
#endif

//---------------------------------------------------------------------------
#define POST_FATAL_ERR_CNT					(0x10)
//---------------------------------------------------------------------------

#define STW_TERMINAL				BIT31
#define STW_COMMAND					BIT30
#define STW_ESC_KEY					BIT29
#define STW_DLE_KEY					BIT28

#define STW_GET_OVERLOAD			BIT27
#define STW_TOF_DONE				BIT26
#define STW_TOF_FAIL				BIT25
#define STW_TOF_PROGRESS			BIT24

#define STW_CONFIG_OK				BIT17
#define STW_POST_OK					BIT16

#if( USE_DATA_UART )
#define STW_DATA_REQUEST			BIT15
#define STW_DATA_FRAME				BIT14
#endif

#define STW_IO_EVENT_STORE 			BIT9
#define	STW_TOF_TESTER 				BIT8
#define	STW_SYSTEM_BUSY 			BIT7
#define	STW_RESET_REQUEST			BIT6

#if( DEV_ISIB3 )
#define	STW_DATABASE_CLEAR_REQUEST 	BIT5
#endif
#if( DEV_TAGXX )
#define	STW_RX_SCAN 				BIT5
#endif


#define STW_CFG_SAVE_REQUEST  		BIT4

#define	STW_BER_REQUEST		BIT3
#define	STW_AIRX_REQUEST	BIT2
#define	STW_UCNT_REQUEST	BIT1
#define	STW_ADC				BIT0

//---------------------------------------------------------------------------
typedef enum
{
    STATE_IDLE,
    STATE_ENERGY_SCANNING,
    STATE_COORDINATOR_STARTED,
    STATE_ACTIVE_SCANNING,
    STATE_ASSOCIATING,
    STATE_ASSOCIATED               // 5

} dev_state_e;


//---------------------------------------------------------------------------
// Битовые поля уровня отладки
#define DL_RXD_CMD	   		BIT0
#define DL_TXD_CMD			BIT1
#define DL_TXD_RXD_BI 		BIT2
#define DL_RXD_FRAME		BIT3

#define DL_CONFIRM_OK		BIT4
#define DL_CONFIRM_BAD		BIT5
#define DL_TOF				BIT6
#define DL_CONFIRM_SYNC		BIT7

//-------------------------------
#define DL_485_PKT          BIT8
#define DL_485_STATE		BIT9
#define DL_485_ERROR		BIT10
#define DL_11				BIT11

//-------------------------------
#define DL_SPI_IO          	BIT12
#define DL_13				BIT13
#define DL_NVR_CACHE		BIT14
#define DL_FRAM_DB 			BIT15

#define DL_GENERIC			BIT24
#define DL_DATA_STATE		BIT25
#define DL_QUALITY			BIT26
#define DL_DATA_UART_TEST	BIT27

#define DL_DUMMY			BIT28


//---------------------------------------------------------------------------
typedef struct
{
    dev_state_e 	state;
    bint    		tx_ack_flag;
    unsigned int   	rx_cnt;
    unsigned int   	ack_cnt;
	void 		  	*mac_handler;
	MAC_Pib_s 		*pib_handler;
#if( DEV_ISPTX )
	union {
	BYTE   	info;
	struct {
	BYTE	batt_low	:1;
	BYTE	tag_type 	:3;
	BYTE	firmware	:4;
	};
	};
#endif
	BYTE			period;
	BYTE			handle_cnt;
} device_data_s;


// Data type for storing data related to all end devices that have associated
typedef struct
{   ushort addr;
    ulong  addr_fl;
    ulong  addr_fh;
} device_list_data_s;


typedef struct
{   // Data related to associated end devices
    int    end_devices;
    device_list_data_s dd[MAX_END_DEVICES];
} end_device_list_s;


//void tx_data_packet( ushort dst_addr, BYTE *data, bint len );
void rx_data_parser_stub( rx_frame_s *f );
void handle_mcps_data( rx_frame_s *f );

void send_bi( bint num_grp, bint cur_grp );
void send_mdm( BYTE *data, bint len );


//void send_bm(void);
//void send_bmx(void);
void send_ttx(void);
//void send_bir( BYTE handle, BYTE features, BYTE *data, BYTE data_len );
void send_bir( BYTE handle, BYTE *data, BYTE data_len );
//void send_bmr( ushort dst_addr, BYTE data );
void send_iir( ushort dst_addr, BYTE request );
void send_ci(void);
void send_cir( ushort dst_addr );
void send_ii( ushort dst_addr, BYTE request );


void confirm_callback( BYTE handle, bint result );
void send_dcr( ushort dst_addr, bint index, ulong config );
void send_dc( ushort dst_addr, bint index, ulong config );
void send_error( ushort dst_addr );


void handle_mcps_confirm( data_confirm_s *ind );


void start_active_scan(void);

void set_rf_state( dev_state_e new_state );
#define set_rf_state_quiet( x ) dd.state = x

void set_my_addr( ushort addr );
ushort get_my_addr(void);
void start_associate(void);



extern device_data_s dd;
//extern end_device_list_s el;

#define pin_lo( x )  		vAHI_DioSetOutput( 0, x )
#define pin_hi( x )  		vAHI_DioSetOutput( x, 0 )
#define pin_do_hi( x ) 		vAHI_DoSetDataOut( x, 0 )
#define pin_do_lo( x ) 		vAHI_DoSetDataOut( 0, x )
#define pin_ctrl( x, y ) 	vAHI_DioSetOutput( x, y )



#define chk_debug( level ) ( cfg.debug & level )
#define dprintf( level, args ... ) if( level & cfg.debug ){ xprintf( args ); }

#define dprintx_header( level )																				\
dprintf( level, "RXD:[%2X] (%3u) S:%4X(%4X) D:%4X(%4X) Len:%3u Seq:%2X Q:%2X\r", f->type_mac, f->param_len, \
													f->src.addr, f->src.panid,                              \
									  				f->dst.addr, f->dst.panid,                              \
													f->len,                                                 \
													f->dsn,                                                 \
									  				f->lq		)

extern bint bi_cod_slots;
//void set_timeslots_cod( bint timeslots );
//BYTE get_timeslots_cod( BYTE ts );
//#define get_timeslots_cod() 	bi_cod_slots


#define led_do_on( x )  	pin_do_hi( x )
#define led_do_off( x ) 	pin_do_lo( x )


#if( ( DEV_TAGXX == 2 )|| DEV_ISPTX )

#define led_on( x )  		pin_lo( x )
#define led_off( x ) 		pin_hi( x )

#define led_red_int_on()  	pin_hi( BIT_LED_RED )
#define led_red_int_off()  	pin_lo( BIT_LED_RED )

#else

#define led_red_int_on()  	pin_do_hi( BIT_LED_DO_RED )
#define led_red_int_off()  	pin_do_lo( BIT_LED_DO_RED )

#endif


#define led_green_on()  	pin_hi( BIT_LED_GREEN )
#define led_green_off()  	pin_lo( BIT_LED_GREEN )



#define pwr_422_on() 		pin_lo( PWR_422_N )
#define pwr_422_off() 		pin_hi( PWR_422_N )

#define out_ctrl( x, y ) 	pin_ctrl( x, y )


#define	make_signature()	\
	{                       \
	txf.signature = dd.pib_handler->u8Dsn;					\
	crc32_add( &txf.signature, (BYTE)(txf.dst.addr>>0) );   \
	crc32_add( &txf.signature, (BYTE)(txf.dst.addr>>8) );   \
	crc32_add( &txf.signature, (BYTE)(cfg.airid.id>>0) );   \
	crc32_add( &txf.signature, (BYTE)(cfg.airid.id>>8) );   \
	crc32_add( &txf.signature, txf.cmd );                   \
	}


/*
#define set_next_handle() 	\
	{                       \
	if( ++bif.handle > HANDLE_xIR_MAX )   					\
		bif.handle = 0;                                     \
	bif.req[bif.handle].utime   = unix_timer + time_stop;   \
	bif.req[bif.handle].bid     = dst_addr;  }              \
*/




#endif
