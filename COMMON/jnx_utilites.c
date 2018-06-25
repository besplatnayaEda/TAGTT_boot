
#include <string.h>

#include "RTOS.h"
#include "jnx_console.h"
#include "jnx_utilites.h"
#include "tof.h"


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint init_beacon(void)
{
bint retcod = 0;
	// Initialise Beacon data

#if( USE_END_DEVICES )
    el.end_devices 	= 0;
#endif

    // Set up the MAC handles.
	// Must be called AFTER u32AppQApiInit()
    dd.mac_handler = pvAppApiGetMacHandle();
    dd.pib_handler = MAC_psPibGetHandle( dd.mac_handler );

    // Set Pan ID and short address in PIB (also sets match registers in hardware)
    MAC_vPibSetPanId( dd.mac_handler, PAN_ID );
    MAC_vPibSetShortAddr( dd.mac_handler, cfg.airid.id );

	if( cfg.airch.out_power & (~CFG_TX_POWER_MASK ) )
	{
// 		vAppApiSetHighPowerMode( APP_API_MODULE_HPM06, TRUE );
   	   vAHI_HighPowerModuleEnable( TRUE, TRUE );
	}

	if( eAppApiPlmeSet( PHY_PIB_ATTR_TX_POWER, cfg.airch.out_power & CFG_TX_POWER_MASK ) != PHY_ENUM_SUCCESS )
	{	printst( "PWR failed " );
		++retcod;
	}

	{
	ulong out_power;
	eAppApiPlmeGet( PHY_PIB_ATTR_TX_POWER, &out_power );
	xprintf( "PWR:%2X", out_power );
	}


	if( eAppApiPlmeSet( PHY_PIB_ATTR_CURRENT_CHANNEL, cfg.airch.ch ) != PHY_ENUM_SUCCESS )
	{	printst( ",CH failed" );
		++retcod;
	}

/*
	// Set Out Power
	if( cfg.airch.out_power & (~CFG_TX_POWER_MASK ) )
//		vAppApiSetHighPowerMode( APP_API_MODULE_HPM06, TRUE );
		vAHI_HighPowerModuleEnable( TRUE, TRUE );
*/



    // Enable receiver to be on when idle
    MAC_vPibSetRxOnWhenIdle( dd.mac_handler, TRUE, FALSE );

    // Allow nodes to associate
    dd.pib_handler->bAssociationPermit = 1;

#if( USE_TOF )
	printst( ",TOF" );
	tof_init();
#endif

	if( sizeof(rx_data_s) != sizeof(MAC_RxFrameData_s) )
	{	println( ",SET failed" );
		++retcod;
	}

	if( cfg.airch.ch )
	{	// Канал выделен жестко - срвзу стартуем
	    start_coordinator( 1 );
	}
	else
	{	set_rf_state( STATE_IDLE );
		start_energy_scan();
	}
	return( retcod );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void start_energy_scan(void)
{
    // Structures used to hold data for MLME request and response
 MAC_MlmeReqRsp_s   req;
 MAC_MlmeSyncCfm_s  sync;

    set_rf_state( STATE_ENERGY_SCANNING );

    // Start energy detect scan
    req.u8Type = MAC_MLME_REQ_SCAN;
    req.u8ParamLength = sizeof( MAC_MlmeReqStart_s );
    req.uParam.sReqScan.u8ScanType = MAC_MLME_SCAN_TYPE_ENERGY_DETECT;
    req.uParam.sReqScan.u32ScanChannels = SCAN_CHANNELS;
    req.uParam.sReqScan.u8ScanDuration = ENERGY_SCAN_DURATION;

    vAppApiMlmeRequest( &req, &sync );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void handle_energy_scan_response( MAC_MlmeDcfmInd_s *ind )
{
int i = 0;
BYTE minenergy;

    minenergy = ind->uParam.sDcfmScan.uList.au8EnergyDetect[0];

    cfg.airch.ch = CHANNEL_MIN;

    // Search list to find quietest channel
    for( i=0; i < ind->uParam.sDcfmScan.u8ResultListSize; i++ )
    {
        if( ind->uParam.sDcfmScan.uList.au8EnergyDetect[i] < minenergy )
        {   minenergy = ind->uParam.sDcfmScan.uList.au8EnergyDetect[i];
            cfg.airch.ch = i + CHANNEL_MIN;
        }
    }

    start_coordinator( 0 );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void start_coordinator( bint quiet )
{
    // Structures used to hold data for MLME request and response
MAC_MlmeReqRsp_s   req;
MAC_MlmeSyncCfm_s  sync;

	if( quiet )
	    set_rf_state_quiet( STATE_COORDINATOR_STARTED );
	else
	    set_rf_state( STATE_COORDINATOR_STARTED );

    // Start Pan
    req.u8Type = MAC_MLME_REQ_START;
    req.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    req.uParam.sReqStart.u16PanId = PAN_ID;
    req.uParam.sReqStart.u8Channel = cfg.airch.ch;
    req.uParam.sReqStart.u8BeaconOrder = 0x0F;
    req.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    req.uParam.sReqStart.u8PanCoordinator = TRUE;
    req.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    req.uParam.sReqStart.u8Realignment = FALSE;
    req.uParam.sReqStart.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest( &req, &sync );
}

#if( USE_END_DEVICES )
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void handle_node_association( MAC_MlmeDcfmInd_s *ind )
{
ushort end_addr;
int idx;

MAC_MlmeReqRsp_s   req;
MAC_MlmeSyncCfm_s  sync;


//!!! Пока заплатка
if( el.end_devices >= MAX_END_DEVICES )
	el.end_devices = 0;


    if( el.end_devices < MAX_END_DEVICES )
    {
        // Store end device address data
        idx = el.end_devices;
        end_addr = END_DEVICE_START_ADDR + el.end_devices;

        el.dd[idx].addr = end_addr;
        el.dd[idx].addr_fl  = ind->uParam.sIndAssociate.sDeviceAddr.u32L;
        el.dd[idx].addr_fh  = ind->uParam.sIndAssociate.sDeviceAddr.u32H;

        el.end_devices++;

        req.uParam.sRspAssociate.u8Status = 0; // Access granted
    }
   	else
   	{   end_addr = 0xFFFF;
       	req.uParam.sRspAssociate.u8Status = 2; // Denied
   	}

    // Create association response
    req.u8Type = MAC_MLME_RSP_ASSOCIATE;
    req.u8ParamLength = sizeof(MAC_MlmeRspAssociate_s);
    req.uParam.sRspAssociate.sDeviceAddr.u32H = ind->uParam.sIndAssociate.sDeviceAddr.u32H;
    req.uParam.sRspAssociate.sDeviceAddr.u32L = ind->uParam.sIndAssociate.sDeviceAddr.u32L;
    req.uParam.sRspAssociate.u16AssocShortAddr = end_addr;

    req.uParam.sRspAssociate.u8SecurityEnable = FALSE;

    // Send association response. There is no confirmation for an association
    //   response, hence no need to check
    vAppApiMlmeRequest( &req, &sync );
}
#endif





//---------------------------------------------------------------------------
// Entry point for application from boot loader. Simply jumps to AppColdStart
// as, in this instance, application will never warm start.
//---------------------------------------------------------------------------
void AppWarmStart(void)
{
    AppColdStart();
}





