
#include "jendefs.h"
#include "mac_sap.h"
#include "AppHardwareApi.h"
#include "jnx_app_buff.h"
#include "AppApi.h"
#include "jnx_common.h"
#include "jnx_console.h"
#include "jnx_utilites.h"

//---------------------------------------------------------------------------
struct mcps_buf_s mcps;
struct mlme_buf_s mlme;

static MAC_DcfmIndHdr_s* mlme_buf_get( void *param );
static void mlme_buf_rx_done( void *param, MAC_DcfmIndHdr_s *buf );

static MAC_DcfmIndHdr_s* mcps_buf_get( void *param );
static void mcps_buf_rx_done( void *param, MAC_DcfmIndHdr_s *buf );

//---------------------------------------------------------------------------
// Initialised the Application API
//---------------------------------------------------------------------------
ulong app_inp_buf_init(void)
{   // Register this layer with AppApi layer
    return( u32AppApiInit( mlme_buf_get, mlme_buf_rx_done, NULL,
                           mcps_buf_get, mcps_buf_rx_done, NULL ) );
}

//---------------------------------------------------------------------------
// Get a buffer pointer. If input is empty, NULL is returned.
//---------------------------------------------------------------------------
MAC_McpsDcfmInd_s *mcps_buf_check(void)
{
	if( mcps.ird == mcps.iwr )
		return( NULL );

	return( (MAC_McpsDcfmInd_s *)&mcps.buf[mcps.ird] );
}

//---------------------------------------------------------------------------
// Free a buffer
//---------------------------------------------------------------------------
void mcps_buf_free( MAC_McpsDcfmInd_s *buff )
{
	if( &mcps.buf[mcps.ird] != (void *)buff )
	{	println( "CPS:Free collision" );
//???
		return;
	}
	mcps.ird = (mcps.ird + 1) & MCPS_BUF_MASK;
}


//---------------------------------------------------------------------------
// Called by lower layer as a callback, this retrieves a buffer from the spare
// buffer queue for MCPS entries, or returns NULL if no buffer is available.
//---------------------------------------------------------------------------
static MAC_DcfmIndHdr_s* mcps_buf_get( void *param )
{
ARGSUSED(param);
bint idx = mcps.idx;

#if( DEV_ISIB3 || DEV_ISBRF || DEV_BRF24 )
   	if( bs_chk( BSF_MODE_WORK|BSF_MODE_PASSIVE|BSF_MODE_SEARCH ) == 0 )
		return( NULL );
#endif

	mcps.idx = (idx + 1) & MCPS_BUF_MASK;
	if( mcps.idx == mcps.ird )
	{   mcps.idx = idx;		// Restore index
		// Âûáðàñûâàòü ìîë÷à, åñëè íå ðàáî÷èå ðåæèìû

		sw_set( STW_GET_OVERLOAD );

		println( "CPS:Get overload" );

		return( NULL );
	}
	return( (MAC_DcfmIndHdr_s *)&mcps.buf[idx] );
}

//---------------------------------------------------------------------------
// Called by lower layer as a callback, this places a buffer onto the used
// buffers queue for MCPS entries.
//---------------------------------------------------------------------------
static void mcps_buf_rx_done( void *param, MAC_DcfmIndHdr_s *buff )
{
ARGSUSED(param);
	if( &mcps.buf[mcps.iwr] != (void *)buff )
	{	println( "CPS:Done collision" );
//???
	}
	mcps.iwr = (mcps.iwr + 1 ) & MCPS_BUF_MASK;
}

//---------------------------------------------------------------------------
// Called by lower layer as a callback, this retrieves a buffer from the spare
// buffers for MLME entries, or returns NULL if no buffer is available.
//---------------------------------------------------------------------------
static MAC_DcfmIndHdr_s* mlme_buf_get( void *param )
{
ARGSUSED(param);
bint idx = mlme.idx;
	mlme.idx = (idx + 1) & MLME_BUF_MASK;
	if( mlme.idx == mlme.ird )
	{   mlme.idx = idx;		// Restore index
		println( "LME:'Get' îverload" );
		return( NULL );
	}
	return( (MAC_DcfmIndHdr_s *)&mlme.buf[idx] );
}


//---------------------------------------------------------------------------
// Called by lower layer as a callback, this places a buffer onto the used
// buffers queue for MLME entries.
//---------------------------------------------------------------------------
static void mlme_buf_rx_done( void *param, MAC_DcfmIndHdr_s *buff )
{
ARGSUSED(param);
	if( &mlme.buf[mlme.iwr] != (void *)buff )
	{	println( "LME:'Done' collision" );
//???
		return;
	}
	mlme.iwr = (mlme.iwr + 1) & MLME_BUF_MASK;
}

//---------------------------------------------------------------------------
// Returns a buffer to the MLME spare buffers
//---------------------------------------------------------------------------
void mlme_buf_free( MAC_MlmeDcfmInd_s *buff )
{
	if( &mlme.buf[mlme.ird] != (void *)buff )
	{  	println( "LME:'Free' collision" );
//???
		return;
	}
	mlme.ird = (mlme.ird + 1 ) & MLME_BUF_MASK;
}

//---------------------------------------------------------------------------
// Reads a buffer pointer from the MLME buffers queue.
// If the queue is empty, NULL is returned.
//---------------------------------------------------------------------------
MAC_MlmeDcfmInd_s *mlme_buf_check(void)
{
	if( mlme.ird == mlme.iwr )
		return( NULL );

	return( (MAC_MlmeDcfmInd_s *)&mlme.buf[mlme.ird] );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void process_mlme_input( MAC_MlmeDcfmInd_s *ind )
{
    // We respond to several MLME indications and confirmations, depending on mode
    switch( ind->u8Type )
    {
	// Incoming association request
    case MAC_MLME_IND_ASSOCIATE:
xprintf( "LME:Assîñiation Request\r" );

#if( USE_END_DEVICES )
        if( dd.state == STATE_COORDINATOR_STARTED )
        {
            handle_node_association( ind );
        }
#endif
        break;

	// Incoming scan results
    case MAC_MLME_DCFM_SCAN:
        if( ind->uParam.sDcfmScan.u8ScanType == MAC_MLME_SCAN_TYPE_ENERGY_DETECT )
        {
xprintf( "LME:Scan Energy Result\r" );

			if( dd.state == STATE_ENERGY_SCANNING )
            {
                // Process energy scan results and start device as coordinator
                handle_energy_scan_response( ind );
            }
        }
		else
			xprintf( "LME:Scan Result(%2X)\r", ind->uParam.sDcfmScan.u8ScanType );

        break;

    case MAC_MLME_IND_COMM_STATUS:
			xprintf( "LME:Comm Status\r" );

		break;

    default:
			xprintf( "LME:%2X Type\r", ind->u8Type );
        break;
    }
}





