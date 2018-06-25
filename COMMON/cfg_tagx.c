
#include "jnx_common.h"
#include "jnx_console.h"
#include "cfg_tagx.h"
#include "tof.h"

eeprom_cfg_u	cfg;


//---------------------------------------------------------------------------
const char *str_ispt_types[8] = {
	"ISPTx",
	"CADDY",
	"TYPE2",
	"TYPE3",
	"IPTxx",
	"TYPE5",
	"TYPE6",
	"TYPE7"
};

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint init_config( bint show )
{
ARGSUSED( show );

bint return_code = 0;
BYTE bytes_per_seg;
ushort segments;

   	printst( "\tInit CFG   " );

	segments = u16AHI_InitialiseEEP( &bytes_per_seg );

	if( bytes_per_seg < sizeof(cfg) )
	{   xprintf( "-Init error:SS:%3u(%3u) N:%6u\r", bytes_per_seg, sizeof(cfg), segments );
		return( 1 );
	}

eeprom_re_read:
	if( return_code >= 3 )
		return( POST_FATAL_ERR_CNT );

	if( eeprom_cfg_read() )
	{	println( "-Reading Error" );
		return( POST_FATAL_ERR_CNT );
	}

	if( ( cfg.signature != CFG_SIGNATURE )||( cfg.airid.id != (cfg.airid.id_sign^0xFFFF) )||(cfg.airch.dev_type != DEV_TYPE) )
//	if( ( cfg.signature != CFG_SIGNATURE )||( cfg.airid.id != (cfg.airid.id_sign^0xFFFF) ) )
	{	// Установки по умолчанию
		cfg.signature 			 = CFG_SIGNATURE;
		cfg.debug 			     = 0;

//#if( DEV_TAGXX )
		cfg.dev_config.time_reg	 = TIME_REREG_DEFAULT; 			// Время перерегитсрации по умолчанию
		cfg.dev_config.list_bif	 = MIN( 5, BIF_BIR_RECORDS ); 	// Количество записей в фильтре биконов

// #if( DEV_TAGXX == 7 )
//		cfg.dev_config.features  = TAG_FEATURES_DATA_REQUEST;
// #else
//  #if( USE_BI_EXTENDED )
		cfg.dev_config.features  = TAG_FEATURES_REREG_SUPPORT;
//  #else
//		cfg.dev_config.features  = 0x00;
//  #endif

// #endif
   		cfg.airch.dev_type  	 = DEV_TYPE;
//#endif

   		cfg.airch.ch  			 = CHANNEL_DEFAULT;
   		cfg.airch.out_power  	 = 0x1F; 	// +2,5db. 0xXX - standart 0x1XX-high power

   		if( cfg.airid.id != (cfg.airid.id_sign^0xFFFF) )
		{
  		cfg.airid.id  		 = BEACON_AIR_MIN_ID;
   		cfg.airid.id_sign  	  	 = cfg.airid.id^0xFFFF;
		}


#if( USE_TOF > 1 )
   		cfg.tof_offset  	 	 = TOF_OFFSET_BASE;

		cfg.mode.m1       = 0;
		cfg.mode.m2  	  = 0;
		cfg.mode.tof_nnn  = TOF_REQUESTS_NNN;
		cfg.mode.tof_mod  = 0x0A;
#endif



		if( eeprom_cfg_store() )
		{	println( "-Fatal Error" );
			return( POST_FATAL_ERR_CNT );
		}
		if( !return_code )
			println( "-Bad Signature - reset to default" );
		return_code++;
		goto eeprom_re_read;
	}

	if( ( cfg.airch.ch < CHANNEL_MIN )||( cfg.airch.ch > CHANNEL_MAX ) )
		cfg.airch.ch = CHANNEL_DEFAULT;

	if( return_code == 0 )
	{	println( "-Ok" );

	}

	cfg_dump();

	return( 0 );
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint cfg_set_air_id( ushort ident )
{
	if( ( ident < TAG_AIR_DEFAULT_ID )||( ident > TAG_AIR_MAX_ID ) )
		return( 1 );

   	cfg.airid.id  	  = ident;
   	cfg.airid.id_sign = ident^0xFFFF;

	// Заявка на запись
	sw_set( STW_CFG_SAVE_REQUEST );
	return( 0 );
}


#if( DEV_TAGXX )

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint cfg_set_config( bint pos, bint value )
{
	switch( pos )
	{
	case 1:
		if( ( value < 2 )||( value > BIF_BIR_RECORDS ) )
			return( 1 );
	  	cfg.dev_config.list_bif = value;
		break;

	case 2:
		if( ( value < 2 )||( value >= 60*30 ) )
			return( 2 );
		cfg.dev_config.time_reg = value;
		break;

	case 3:
		if( value > 0xFF )
			return( 3 );
		cfg.dev_config.features = value;
		break;

	default:
		return( 3 );
	}
	// Заявка на запись
	sw_set( STW_CFG_SAVE_REQUEST );
	return( 0 );
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint cfg_set_config_full( tag_config_u config )
{
	if( ( config.list_bif < 2 )||( config.list_bif > BIF_BIR_RECORDS ) )
		return( 1 );

	if( ( config.time_reg < 2 )||( config.time_reg >= 60*30 ) )
		return( 2 );

  	cfg.dev_config.full = config.full;

	// Заявка на запись
	sw_set( STW_CFG_SAVE_REQUEST );
	return( 0 );
}

#endif

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void cfg_dump(void)
{
	xprintf( "\t\t  Ident: %2X:%5u(%4X) Ch:%2u:%4X Power:%2X\r", cfg.airch.dev_type, cfg.airid.id, cfg.airid.id, cfg.airch.ch, PAN_ID, cfg.airch.out_power );
   	xprintf( "\t\tTimeout: %5u LS:%2X (%8X)\r" , cfg.dev_config.time_reg, cfg.dev_config.list_bif, cfg.dev_config.full );
	xprintf( "\t\t    Ext: %2X\r", cfg.dev_config.features  );
#if( USE_TOF > 1 )
	xprintf( "\t\t    TOF: NN:%3u, Mode:%2X, Offset:%6u\r", cfg.mode.tof_nnn, cfg.mode.tof_mod, cfg.tof_offset  );
#endif
	xprintf( "\t\t  Debug: %8X\r",  cfg.debug  );
}


