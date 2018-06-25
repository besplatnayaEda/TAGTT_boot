

#include <string.h>

#include "jnx_common.h"
#include "jnx_console.h"


#include "xtag_protocol.h"
#include "crc32.h"


static void tx_frame( tx_frame_s *f, BYTE handle );

device_data_s 		dd;

tx_frame_s			txf;

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void set_rf_state( dev_state_e new_state )
{
    dd.state = new_state;
	switch( dd.state )
	{
	case STATE_ASSOCIATED:
		xprintf( "SYS:ID:%4X Ch:%2u Associated\r", cfg.airid.id, cfg.airch.ch );
		break;

	case STATE_COORDINATOR_STARTED:

		xprintf( "SYS:ID:%4X Ch:%2u Coordinator Started\r", cfg.airid, cfg.airch.ch );
		break;

	default:
		xprintf( "SYS:ID:%4X Ch:%2u Device State:%1X\r", cfg.airid.id, cfg.airch.ch, dd.state );

	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
static void tx_frame( tx_frame_s *f, BYTE handle )
{
data_confirm_s cfm;

    // Create frame transmission request
    f->type_mac = MAC_MCPS_REQ_DATA;
    // Use short address for source
    f->src.addr_mode = 2;
   	f->src.panid = PAN_ID;
    f->src.addr  = cfg.airid.id;
    // Use short address for destination
    f->dst.addr_mode = 2;
   	f->dst.panid = PAN_ID;
    // Frame requires ack but not security, indirect transmissionnsmit or GTS
	if( f->dst.addr == AIR_BROADCAST_ID )
	{	f->options = 0;
	    // Set handle so we can match confirmation to request

    	f->handle = 0xFF; 	// Признак того, что ACK будет безусловным
	}
	else
	{  	f->options = MAC_TX_OPTION_ACK;
    	f->handle  = handle;
	}

	if( chk_debug( DL_TXD_CMD ) )
	{   if( f->cmd == CMD_BI )
		{ 	if( chk_debug( DL_TXD_RXD_BI ) )
				xprintf( "CMD<  :%4X BI    (L=%3u)\r", f->dst.addr, f->cmd, f->len );
		}
		else
			xprintf( "CMD<  :%4X CMD%2X (L=%3u) [%2X]\r", f->dst.addr, f->cmd, f->len, handle );

	}
    f->sd.u8SecurityLevel = MAC_SECURITY_NONE;

    // Request transmit
	if( chk_debug( DL_CONFIRM_SYNC ) )
    	vAppApiMcpsRequest( (MAC_McpsReqRsp_s *)f, (MAC_McpsSyncCfm_s *)&cfm );
	else
		vAppApiMcpsRequest( (MAC_McpsReqRsp_s *)f, NULL );

	if( chk_debug( DL_CONFIRM_SYNC ) )
	{	if( cfm.handle != HANDLE_BROADCAST )	// Получено НЕ безусловное подтверждение при не BROADCAST - отображаем
			xprintf( "CFM:[%2X] %2X/%2X (%3u) \r", cfm.handle, cfm.type_mac, cfm.status, cfm.param_len );
	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void handle_mcps_confirm( data_confirm_s *f )
{

	switch( f->status )
	{
	case MAC_ENUM_SUCCESS:
        // Data frame transmission successful
		if( f->handle != HANDLE_BROADCAST )	// НЕ безусловное подтверждение при BROADCAST
		{   confirm_callback( f->handle, f->status );

printf( "TXD:%2X [%2X|%2X] (%3u) Ok\r", f->handle, f->type_mac, f->status, f->param_len );
			dprintf( DL_CONFIRM_OK, "TXD:%2X [%2X|%2X] (%3u) Ok\r", f->handle, f->type_mac, f->status, f->param_len );
		}
		else
		{
			dprintf( DL_CONFIRM_OK, "TXD:Broadcast\r" );
		}
		break;

    case MAC_ENUM_NO_ACK:

		confirm_callback( f->handle, f->status );
    	// Data frame transmission unsuccessful

		dprintf( DL_CONFIRM_BAD, "TXD:%2X [%2X|%2X] (%3u) No ACK\r", f->handle, f->type_mac, f->status, f->param_len );
		break;

	case MAC_ENUM_CHANNEL_ACCESS_FAILURE:
		confirm_callback( f->handle, f->status );
		dprintf( DL_CONFIRM_BAD, "TXD:%2X [%2X|%2X] (%3u) CH Access Failure\r", f->handle, f->type_mac, f->status, f->param_len );
		break;

	default:
		confirm_callback( f->handle, f->status );
		xprintf( "TXD:%2X [%2X|%2X] (%3u) Faled\r", f->handle, f->type_mac, f->status, f->param_len );
    }
}


//---------------------------------------------------------------------------
//    BYTE          type_mac;         					// Indication type (@sa MAC_McpsDcfmIndType_e)
//    BYTE          param_len;   						// Parameter length in following union
//    ushort        x_pad;         						// Padding to force alignment
//    mac_addr_s 	src;                                // Source address
//    mac_addr_s 	dst;                                // Destination address
//    BYTE    		lq;                           		// Link quality of received frame
//    BYTE      	sec_flag;                           // TRUE if security was used
//    BYTE      	acl_entry;                          // Security suite used
//    BYTE      	len;                         		// Length of payload (MSDU)
//
//    BYTE      	d[MAC_MAX_DATA_PAYLOAD_LEN];    	// Payload (MSDU)
//
//    // Mac 2006 additions
//    BYTE      				dsn;                    // Data Sequence Num of Rx frame
//    ulong     				timestamp;             	// Rx frame timestamp
//    MAC_SecurityData_s      	sd;          			// Security Data (Info)
//
//	Простая ничего не делающая заглушка может быть использована для распечатки
//  неизвестных фреймов
//---------------------------------------------------------------------------

void handle_mcps_data( rx_frame_s *f )
{
	dprintx_header( DL_RXD_FRAME );
//dprintf( DL_RXD_HDR, "RXD:[%2X] (%3u) S:%4X(%4X) D:%4X(%4X) Len:%3u Seq:%2X Q:%2X\r", f->type_mac, f->param_len,
//													f->src.addr, f->src.panid,
//									  				f->dst.addr, f->dst.panid,
//													f->len,
//													f->dsn,
//									  				f->lq		);

}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void send_bir( BYTE handle, BYTE *data, BYTE data_len )
{
	if( data_len > TAG_MAX_ADD_DATA_LEN )
		data_len = TAG_MAX_ADD_DATA_LEN;

//	// Запомнили 'фичи' бикона для ответа. Например для бикона ламповой будет другой таймаут
//    bif.rec[handle].features = features;

	txf.cmd  					= CMD_BIR;
	txf.dev_type 				= cfg.airch.dev_type;
    txf.dev_requests 			= 0;
	txf.dev_features 			= cfg.dev_config.features;
    txf.dst.addr 				= bif.rec[handle].addr;

	make_signature();

	if( data_len )
		memcpy( &txf.bir.data[0], data, data_len );

    txf.len = sizeof(air_header_s) + data_len;

	tx_frame( &txf, handle );
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void send_error( ushort dst_addr )
{
	txf.cmd  			= CMD_ERROR;
	txf.dev_type 		= cfg.airch.dev_type;
    txf.dev_requests 	= 0;
	txf.dev_features 	= cfg.dev_config.features;
    txf.dst.addr 		= dst_addr;

	make_signature();

    txf.len = sizeof(air_header_s);
	tx_frame( &txf, HANDLE_ERROR );
}




