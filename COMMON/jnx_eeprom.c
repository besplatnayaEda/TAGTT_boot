
#include "jnx_common.h"
#include "jnx_console.h"

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int init_eeprom( bint show )
{
BYTE bytes_per_seg;
ushort segments = u16AHI_InitialiseEEP( &bytes_per_seg );

	if( bytes_per_seg < sizeof(eeprom_cfg_u) )
	{   xprintf( "\tEEP  :SS:%3u(%3u) N:%6u - Invalid\r", bytes_per_seg, sizeof(eeprom_cfg_u), segments );
		return( 1 );
	}

	return(	init_config( show ) );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint eeprom_cfg_store(void)
{
	if( iAHI_WriteDataIntoEEPROMsegment( CFG_EEPROM_SEGM, CFG_EEPROM_ADDR, &cfg.bval[0], sizeof(eeprom_cfg_u) ) )
	{   println( "\tEEP  :Write Error");

		return( 3 );
	}
	return( 0 );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint eeprom_cfg_read(void)
{
	if( iAHI_ReadDataFromEEPROMsegment( CFG_EEPROM_SEGM, CFG_EEPROM_ADDR, &cfg.bval[0], sizeof(eeprom_cfg_u) ) )
	{   println( "\tEEP  :Read Error" );
		return( 2 );
	}
	return( 0 );
}


