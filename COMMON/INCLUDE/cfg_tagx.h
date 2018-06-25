//---------------------------------------------------------------------------
//  Конфигурационные данные для Тагов
//  Release 0.1
//---------------------------------------------------------------------------

#ifndef __EEPROM_XTAG_H_
#define __EEPROM_XTAG_H_

#ifdef __cplusplus
extern "C" {
#endif

//#define CFG_ELEMENT_SIZE  		(4)
//#define CFG_MAX_ELEMENT_N 		(8)
#define CFG_EEPROM_ADDR	  		(0)		//
#define CFG_EEPROM_SEGM	  		(0)		//  Для JNxx
#define	CFG_MAX_WORDS			(8)

#define CFG_TX_POWER_MASK  	   	(0x3F)



//#define MAX_CFG_SIZE		((CFG_ELEMENT_SIZE)*(CFG_MAX_ELEMENT_N))

//---------------------------------------------------------------------------
#pragma pack( push, 1 )


//---------------------------------------------------------------------------
// Слово конфигурации Эфирного канала
typedef struct {
		BYTE	ch;
		BYTE	dev_type;     		// Тип устройства (TAG_TYPE/BEACON_TYPE)
		BYTE  	_reserv;
		BYTE  	out_power;
	} air_ch_config_s;

//---------------------------------------------------------------------------
// Слово конфигурации Эфирного ID
typedef struct {
		ushort	id;
		ushort  id_sign;
	} air_id_config_s;


//---------------------------------------------------------------------------
typedef	union {
	ulong	full;       // 4
	struct {
	ushort	time_reg;
	BYTE	list_bif;
	BYTE	features;
	};
} tag_config_u;


//---------------------------------------------------------------------------
// Структура конфигурации лежащая в EEPROM
typedef struct {
	ulong	signature;      			// 0
	ulong	debug;         				// 1

	union {
	air_ch_config_s		airch;			// 2 Канал
	ulong				airch_long;
	};

	union {
	air_id_config_s		airid; 			// 3 ID
	ulong				airid_long;
	};

//#if( DEV_ISPTX )
//	union {
//	mrk_id_config_s		mrkid; 			// 4
//	ulong			    mrkid_long;
//	};
//#elif( DEV_TAGXX )
	tag_config_u  		dev_config;     // 4
//#else
//#error "No Tag Type"
//#endif
	ulong  				tof_offset;  	// 5

#if( USE_TOF > 1 )
	union  {
	struct {
	BYTE				m1;           	// 6
	BYTE				m2;
	BYTE				tof_nnn;
	BYTE				tof_mod;
	} mode;
	ulong				tof_config;
	};

#endif


//	ulong  				_res_6;  		// 6
//	ulong  				_res_7;  		// 7
} eeprom_cfg_s;

#pragma pack( pop )


typedef union {
	eeprom_cfg_s;
	BYTE 		bval[CFG_MAX_WORDS*4];
	ulong 		word[CFG_MAX_WORDS];
} eeprom_cfg_u;


//---------------------------------------------------------------------------
extern eeprom_cfg_u	cfg;

int init_eeprom( bint show );
//bint init_eeprom_config(void);
bint eeprom_cfg_store(void);
bint eeprom_cfg_read(void);

bint init_config( bint show );
#define cfg_read()   eeprom_cfg_read()
#define cfg_store()  eeprom_cfg_store()
void cfg_dump(void);
//bint cfg_set_beacon_address( ushort address );
//bint cfg_set_serialn( ushort serialn );
//bint cfg_set_baud( ulong baud );
bint cfg_set_air_id( ushort ident );
bint cfg_set_mrk_id( ushort ident );
//bint cfg_set_list_size( bint list_size );
bint cfg_set_config_full( tag_config_u config );
bint cfg_set_config( bint pos, bint value );


#ifdef __cplusplus
}
#endif


#endif

