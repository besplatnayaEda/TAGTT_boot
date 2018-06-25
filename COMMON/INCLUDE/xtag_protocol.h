
#ifndef __PROTOCOL_TAG_AK_H
#define __PROTOCOL_TAG_AK_H

#define USE_BI_EXTENDED				(1)

#define PAN_ID         				(0x495A)
#define BEACON_AIR_ID_FLAG      	(0xC000)
#define BEACON_AIR_MIN_ID      		BEACON_AIR_ID_FLAG  // Стартовый (младший начальный) Air ID биконов
#define ISPTX_AIR_FIXED_ID         	(0x00FF)
#define ISPT_DEFAULT_MARK_ID        (0x0100)
#define TAG_AIR_DEFAULT_ID         	(0x0100)    //
#define ISPT_DEFAULT_MARK_ID        (0x0100)
#define TAG_AIR_MIN_ID         		(0x00FF)    //
#define TAG_AIR_MAX_ID         		(ushort)(BEACON_AIR_MIN_ID-1)  // Ниже зоны адресов биконов
#define AIR_BROADCAST_ID           	(0xFFFF)
#define MAX_END_DEVICES             (16)
#define END_DEVICE_START_ADDR       (AIR_BROADCAST_ID - MAX_END_DEVICES - 1)
// Defines the channels to scan. Each bit represents one channel. All channels
//   in the channels (11-26) in the 2.4GHz band are scanned
#define SCAN_CHANNELS               (0x07FFF800UL)
#define CHANNEL_MIN                 (11)
#define CHANNEL_MAX                 (26)
#define CHANNEL_DEFAULT             (16)
#define CHANNEL_DEFAULT_MODEM       (25)


#define CMD_BI				(0x01) 		// Broadcast invitation command
#define CMD_BIR		    	(0x02) 		// Broadcast invitation reply command
#define CMD_ERROR           (0x03)      // Неподдерживаемая тагом команда
#define CMD_II	    		(0x04)      // Individual Invite
#define CMD_IIR	    		(0x05)      // Individual Invite Reply
#define CMD_DC				(0x06) 		// Device Configuration Set
#define CMD_DCR  			(0x07) 		// Tag Configuration Set Reply
//                          (0x08)
#define CMD_TTX				(0x09) 		// Dummy Tag TX
//#define CMD_BTX				(0x0A) 		// Dummy Beacon TX
#define CMD_BM				(0x0B) 		// Broadcast Beacon Mark command
//#define CMD_BMX				(0x0C) 		// Broadcast Beacon Mark command + Reply Request
//#define CMD_BMR				(0x0D) 		// Broadcast Beacon Mark command Reply
#define CMD_CI				(0x0E) 		// Collision Invite
#define CMD_CIR				(0x0F) 		// Collision Invite Reply
#define CMD_MDM				(0x1F) 		// Modem Frame




#define BIF_STATE_SEND_DATA		(3)
#define BIF_STATE_REQ			(2)
#define BIF_STATE_WAIT_DATA		(1)
#define BIF_STATE_WAIT			(0)
#define BIF_STATE_WAIT_DATA_ERROR	(4)


#define CI_COMMAND_100MS_SLOTS_DEFAULT    	(20)
#define BI_COMMAND_TIMESLOTS_COD_DEFAULT    (6)
#define II_COMMAND_TIMESLOTS    			(12)
#define II_COMMAND_RETRY_COUNTER			(3)
#define TOF_COMMAND_TIMESLOTS    			(120)


#pragma pack( push, 1 )

//---------------------------------------------------------------------------
// Обязательный заголовок всех эфирных пакетов, кроме автономных тагов и отметчиков. У автномных 16bit Header c 8bit сигнатурой
// для экономии
typedef struct air_header_s {
	BYTE    cmd;            // Команда
	BYTE	dev_type;
	union	{
	struct	{
	BYTE	dev_requests;
	BYTE    dev_features;
	};
	ushort	dev_info_union;
	};
#define DF_LAMP_ROOM			(BIT0)
#define DF_TRAFFIC_CONTROLER	(BIT7)

	ulong	signature;      // Контрольная подпись пакета
} air_header_s;


#define TAG_MAX_RAW_FRAME_LEN	(112)
#define TAG_MAX_ADD_DATA_LEN	((TAG_MAX_RAW_FRAME_LEN) - sizeof(air_header_s))


//---------------------------------------------------------------------------
typedef struct gas_station_result_s {
	ushort	value;
	BYTE	name;
	BYTE    unit;
} gas_station_result_s;

//---------------------------------------------------------------------------
typedef struct gas_station_state_s {
	BYTE	level1;
	BYTE	level2;
	BYTE    failed;
	BYTE    reserv;
} gas_station_state_s;

//---------------------------------------------------------------------------
// Данные полученные в ответ на индивидуальный запрос тага или иного оборудования
typedef union {
	struct {
	BYTE 	ext_sysversion;
	BYTE	ext_subversion;
	BYTE	ext_1;
	BYTE	ext_0;
	};
	gas_station_result_s	gsd;
	gas_station_state_s     gss;
	ulong	ext_full_long;
} iir_data_u;


//---------------------------------------------------------------------------
// Структура идентификатора Тага
typedef union f_tag_id_u {
	ulong	tid;
	struct {
	BYTE	tid_type;
	BYTE	tid_ext_ident;
	ushort	tid_addr;
	};
} f_tag_id_u;


#if defined( __ICCARM__ )
#else



//---------------------------------------------------------------------------
#define	DEV_TYPE_WIDE	(sizeof(BYTE))
#define	DEV_FUNC_SHIFT 	(sizeof(ulong)-DEV_TYPE_WIDE)
#define	DEV_FUNC_WIDE 	(DEV_FUNC_SHIFT)
//
//---------------------------------------------------------------------------
// Beacon Invite даные
typedef struct bi_data_s {
   	BYTE   	cod_group		:3;
   	BYTE    reserv_12  		:1;
   	BYTE    cod_slots   	:4;
	BYTE   	reserv_7  		:1;
	BYTE   	cur_group    	:7;
//#if( USE_BI_EXTENDED )
//	BYTE	reg_timeout;
//#endif

} bi_data_s;

#if( USE_BI_EXTENDED )

//---------------------------------------------------------------------------

typedef struct bi_short_s{
	struct air_header_s;
	bi_data_s;
} bi_short_s;

//---------------------------------------------------------------------------
typedef struct bi_traffic_s{
	struct air_header_s;
	union {
	bi_data_s;
	ushort data_raw;
	};
	BYTE	reg_timeout;
	BYTE	traffic[12];
} bi_traffic_s;

#endif

typedef struct bi_s{
	struct air_header_s;
	union {
	bi_data_s;
	ushort data_raw;
	};
#if( USE_BI_EXTENDED )
	BYTE	reg_timeout;
#endif
} bi_s;


//---------------------------------------------------------------------------
typedef struct ii_s{
	struct air_header_s;
// Битовые флаги поля dev_requests
#define	II_REQ_STATE		BIT0     // Совпадает с Командой TII
#define	II_REQ_DATA			BIT1     // Совпадает с Командой TII
#define	II_SET_DIRECTION	BIT2     // Передача IVT направления движения в дополнительном байте
} ii_s;

//---------------------------------------------------------------------------
// Пока только для передачи данных о направлении движения для IVT24
typedef struct ii_ext_s{
	struct  air_header_s;
	BYTE	data;
} ii_ext_s;

//---------------------------------------------------------------------------
typedef struct {
	air_header_s;
	iir_data_u;
} iir_s;

//---------------------------------------------------------------------------
typedef struct dc_s{
	struct air_header_s;
	// В заголовке air_header_s поле dev_requests содержит номер конфигурируемого параметра
	// Конфигурируемые данные в 'config'. Если соnfig 0xFFFFFFFF, то в ответ будет выдана текущая конфигурация
	ulong  config;
	ulong  config_sign;
} dc_s;

//---------------------------------------------------------------------------
typedef struct dcr_s{
	struct air_header_s;
	// В заголовке air_header_s поле dev_requests содержит номер запрашиваемого параметра
	ulong  config;
} dcr_s;


//---------------------------------------------------------------------------
// Пакет Автономного отметчика
typedef struct bm_s {
	BYTE	cmd;
	BYTE   	info;
	ushort	ident;
	BYTE   	signature;
} bm_s;

//---------------------------------------------------------------------------
// Пакет modems
typedef struct mdm_s {
	BYTE	cmd;
	BYTE   	data[1];
} mdm_s;

//---------------------------------------------------------------------------
// Пакет Тага-Дятла
typedef struct ttx_s {
	BYTE	cmd;
	union {
	BYTE   	info;
	struct {
	BYTE	batt_low	:1;
	BYTE	tag_type 	:3;
	BYTE	firmware	:4;
	};
	};
	ushort	ident;
	BYTE   	signature;
} ttx_s;

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
typedef struct {
	air_header_s;
	BYTE	data[TAG_MAX_ADD_DATA_LEN];
} bir_s;

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
typedef union xtag_protocol_u{
	air_header_s;
	bm_s			bm;
	ttx_s			ttx;
	bi_s			bi;
	bi_traffic_s   	bi_traffic;
	bir_s			bir;
	ii_s			ii;
	ii_ext_s  		ii_ext;
	dc_s			dc;
	dcr_s			dcr;
	iir_s			iir;
	mdm_s			mdm;
} xtag_protocol_u;

/*
//---------------------------------------------------------------------------
// Структура идентификатора Тага
typedef union f_tag_id_u {
	ulong	tid;
	struct {
	BYTE	tid_type;
	BYTE	tid_ext_ident;
	ushort	tid_addr;
	};
} f_tag_id_u;
*/


#pragma pack( pop )

//---------------------------------------------------------------------------
#define HANDLE_DEFAULT			(0x80)
#define HANDLE_BROADCAST		(0xFF)

//
#define BIF_BIR_RECORDS	 		(8)
#define HANDLE_INVALID   		(-1)
#define HANDLE_BIR_MAX   		((BIF_BIR_RECORDS)-1)
#define HANDLE_IIR              ((HANDLE_BIR_MAX)+1)
#define HANDLE_CIR              ((HANDLE_BIR_MAX)+2)
#define BIF_NNN_RECORDS         ((BIF_BIR_RECORDS)+1)


#define HANDLE_ERROR            ((HANDLE_DEFAULT)+1)

 #if( DEV_TAGXX )
 #else
#define HANDLE_II               ((HANDLE_DEFAULT)+2)
#define HANDLE_II_NO_REPLY      ((HANDLE_DEFAULT)+3)
#define HANDLE_MODEM      		((HANDLE_DEFAULT)+4)
 #endif



 #if( DEV_TAGXX || DEV_IVT24 )
//---------------------------------------------------------------------------
// Структура записи фильра BI
typedef	struct bif_record_s {
	ushort		addr;           // Адрес бикона
	int			cod_slots; 	 	// Код Количества таймслотов бикона
	volatile int			state;			// Состояние бикона
	int			flags; 	 		// Дополнительные флаги к state
	int			cnt_bi;  		// BI Counter
	int			cnt_bir;  		// BIR Retry Counter
  #if( USE_DATA_UART )
	int			cnt_bi_timeout; // Счетчик таймаута в запросах BI. Используется для ожидания данных от газоанализатора
  #endif
#if( USE_BI_EXTENDED )
	int			reg_timeout;
#endif
	BYTE		features;
} bif_record_s;


// Структура данных фильтра BI (BI Filter)
typedef	struct bif_data_s {
	int				idx_max;      	 	// Максимальный.
    int				idx_best;      	 	// Текущий наилучший
	int	  			idx_handle; 		// Последий отвеченный
	int	  			idx_handle_data;
	int	  			rec_cnt; 				// Количество активных записей
	int  			quality; 				// Вычисленный критерий качества (c учетом форы) для текущего бикона
	bif_record_s	rec[BIF_NNN_RECORDS];  //
} bif_data_s;
 #endif


//---------------------------------------------------------------------------
// Аналог юниона MAC_Addr_u
//---------------------------------------------------------------------------
typedef union mac_addr_u {
    ushort addr;     			// Short address
    MAC_ExtAddr_s addr_ext;  	// Extended address
} mac_addr_u;

//---------------------------------------------------------------------------
// Аналог структуры MAC_Addr_s
//---------------------------------------------------------------------------
typedef struct mac_addr_s {
    BYTE      addr_mode;  	// Address mode
    ushort    panid;    	// PAN ID
	mac_addr_u;
} mac_addr_s;


//---------------------------------------------------------------------------
// Копия структуры MAC_RxFrameData_s
// Долбоебы описавшие эти структуры не потрудились правила паковки описать.
// По умолчанию паковка странная - байты пакуются встык, остальные выравнивабтся по 4
// при замене массива на union со структурами паковка по умолчанию ломается,
// посему добавления #pragma pack()
//---------------------------------------------------------------------------
#pragma pack( push, 2 )
typedef struct {
    mac_addr_s 			src;          				// Source address
    mac_addr_s 			dst;                        // Destination address
    BYTE    			lq;                    		// Link quality of received frame
    BYTE      			sec_flag;                   // TRUE if security was used
    BYTE      			acl_entry;                  // Security suite used
    BYTE      			len;                   		// Length of payload (MSDU)

	union{
    BYTE      			buff[MAC_MAX_DATA_PAYLOAD_LEN];    	// Payload (MSDU)
	union 				xtag_protocol_u;
	};
    // Mac 2006 additions
    BYTE      				dsn;                    // Data Sequence Num of Rx frame
    ulong     				timestamp;             	// Rx frame timestamp
    MAC_SecurityData_s      sd;          			// Security Data (Info)

} rx_data_s;
#pragma pack( pop )



#pragma pack( push, 4 )
//--------------------------------------------------------------------------
// Аналог структуры MAC_TxFrameData_s;
//--------------------------------------------------------------------------
typedef struct {
    mac_addr_s         	src;                        // Source address
    mac_addr_s         	dst;                        // Destination address
    BYTE               	options;                    // Transmit options (MAC_TransmitOption_e)
    BYTE               	len;                        // Length of payload (MSDU)
    MAC_SecurityData_s 	sd;                       	// Security Information
	union {
    BYTE        		raw_mac_buff[MAC_MAX_DATA_PAYLOAD_LEN];  // Payload (MSDU)
	union 			  	xtag_protocol_u;
	};
} tx_data_s;
#pragma pack( pop )


//---------------------------------------------------------------------------
// Data transmit confirm. Use type MAC_MCPS_CFM_DATA
// Аналог MAC_McpsCfmData_s;
//---------------------------------------------------------------------------
typedef struct {
    BYTE  handle; 		// Handle matching associated request
    BYTE  status; 		// Status of request @sa MAC_Enum_e
    ulong timestamp; 	// Time in symbols which the data was transmitted (not supported)
} data_confirm_result_s;

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
typedef struct {
    BYTE               	type_mac;   	// Indication type (@sa MAC_McpsDcfmIndType_e)
    BYTE             	param_len;   	// Parameter length in following union
    ushort            	x_pad;         	// Padding to force alignment
    rx_data_s;          		   		// Data frame
} rx_frame_s;


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
typedef struct tx_frame_s
{
    BYTE             	type_mac;      	// Request type (@sa MAC_McpsReqRspType_e)
    BYTE               	param_len;  	// Parameter length in following union
    ushort              x_pad;         	// Padding to force alignment
//  MAC_McpsReqRspParam_u uParam;     	// Union of all possible Requests
    BYTE            	handle; 		// Handle of frame in queue
    BYTE             	ch; 			// Extension to support TX on specific
                                    	// channel. Use MAC_MCPS_REQ_DATA_EXTENDED to
                                    	// enable this feature, otherwise ignored for
                                    	// backward compatibility
    tx_data_s;          				// TX Data Frame

} tx_frame_s;


//---------------------------------------------------------------------------
// 1) MCPS Deferred Confirm/Indication (MAC_McpsDcfmInd_s)
// The object passed in the MCPS Deferred Confirm/Indication callback
// 2) MCPS Synchronous Confirm (MAC_McpsSyncCfm_s)
// The object returned by MAC_vHandleMcpsReqRsp containing the synchronous confirm.
// The confirm type is implied as corresponding to the request
// All Confirms may also be sent asynchronously via the registered Deferred Confirm/Indication callback;
// this is notified by returning MAC_MCPS_CFM_DEFERRED.
//---------------------------------------------------------------------------
typedef struct data_confirm_s
{   BYTE                  type_mac;    	  	// Confirm status (@sa MAC_McpsSyncCfmStatus_e)
    BYTE                  param_len;   		// Parameter length in following union
    ushort                pad_x;          	// Padding to force alignment
//  MAC_McpsSyncCfmParam_u uParam;        	// Union of all possible Confirms
    data_confirm_result_s;

}	data_confirm_s;


void rx_data_parser( rx_frame_s *f );

#endif




#if( DEV_TAGXX || DEV_IVT24 )
void bif_init(void);
extern bif_data_s  bif; 			// BI Filter operation data
#endif



//---------------------------------------------------------------------------
// Данные для команды II
typedef struct ii_data_s {
    f_tag_id_u;
	BYTE			ext;
#if( DEV_ISIB2 )
	BYTE			cmd;
#else
	volatile bint   req_counter;
#endif
} ii_data_s;


// Флаги состояния канала
extern ii_data_s	iid;



void bif_list(void);
void chk_registration(void);
void set_uart_data_ready(void);
void set_uart_data_failed(void);
int  set_data_request( BYTE handle );
int  set_uart_data_request( BYTE handle, ushort flags );
void modem_data( BYTE *buff, bint len, BYTE lq );
void modem_check(void);
void modem_send( BYTE *data, bint len );
void modem_resend(void);
void modem_ack(void);




#define TAG_FEATURES_DATA_REQUEST		(BIT7)
#define TAG_FEATURES_LIVE_DISABLE		(BIT6)
#define TAG_FEATURES_REREG_SUPPORT		(BIT0)

#endif


