//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------

#include <mac_sap.h>
#include <mac_pib.h>
#include <stdint.h>

#include "jnx_common.h"
#include "config.h"
#include "jnx_app_buff.h"
#include "jnx_console.h"
#include "cfg_tagx.h"
#include "jnx_utilites.h"
#include "tof.h"
#include "jnx_leds2.h"
//#include "uart_data.h"
//#include "fram.h"
//#include "lcd_driver.h"
//#include "jnx_button.h"


//---------------------------------------------------------------------------
void init_tag_system(void);
void hello_message(void);
//static void process_queues(void);
void process_queues(void);
static void process_mcps_input( MAC_McpsDcfmInd_s *buf );

//---------------------------------------------------------------------------
// Entry point for application from boot loader. Initialises system and runs
// main loop
bint led_blink_counter;

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//	bootloader
//---------------------------------------------------------------------------

// проверка наличия прошивки
int CheckFWPresent(void);



// проверка наличия прошивки
int FWPresent;


//---------------------------------------------------------------------------

void AppColdStart(void)
{
    // Disable watchdog if enabled by default
    vAHI_WatchdogStop();

    init_tag_system();


	if( cfg.airid.id )
	{	// Канал выделен жестко - срвзу стартуем
	    start_coordinator( TRUE );
	}
	else
	{	set_rf_state( STATE_IDLE );
		start_energy_scan();
	}

	vAHI_WatchdogStart( 9 );  	// 4096ms



    for( ; ; )
	{

#if( USE_TOF > 1 )
		if( sw_chk( STW_TOF_TESTER ) )
		{	tof_tester();
			continue;
		}
#endif

		process_queues();


#if( USE_TOF > 1 )
		if( sw_chk( STW_TOF_DONE|STW_TOF_PROGRESS ) )
		{  	// Процедура измерения расстояния запущена. Ждать завершения
			if( sw_chk( STW_TOF_DONE ) )
			{
//				led_red_int_off();

				switch( process_tof_double( cfg.debug & DL_TOF ) )
				{

				case 1:
					// Первый проход
					sw_clr( STW_TOF_DONE );

					break;

				case 2:
					// Второй или единственый проход

				case 0:
				// Ошибка

					sw_clr( STW_TOF_DONE|STW_TOF_PROGRESS );
//					bs_clr( BSF_MODE_TOF_REQUEST )
					break;
				}

			}
			continue;
		}
#endif


		//-------------------------------------------------------------------
		// 100ms Таймер
		if( chk_timer_restart( &ledx_timer ) )
		{	// 100ms моргалка светодиодами
			led_process( led_blink_counter++ );
		}

		//-------------------------------------------------------------------
//		// Таймер перегистрации и супервизор
//		chk_registration();

		//-------------------------------------------------------------------
		// Односекундный таймер
		if( unix_timer != unix_timer_cnt )
		{	unix_timer =  unix_timer_cnt;

		   	if( sw_chk( STW_CFG_SAVE_REQUEST | STW_RESET_REQUEST ) )
			{   // Запись в EEPROM
				if( sw_chk( STW_CFG_SAVE_REQUEST ) )
				{	sw_clr( STW_CFG_SAVE_REQUEST );
					if( eeprom_cfg_store() == 0 )
				   		println( "CFG:Stored\x12" );   	//+ DC2
				}
				else if( sw_chk( STW_RESET_REQUEST ) )
					reset( 0 );

			}

			vAHI_WatchdogRestart();
		}


		//-------------------------------------------------------------------
		if( sw_chk( STW_COMMAND ) )
		{ 	vAHI_WatchdogRestart();
			cmd_parser();
			sw_clr( STW_COMMAND );			// Clear request
		}
#if( USE_POWER_SLEEP )
		vAHI_CpuDoze();
#endif

	}
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void init_tag_system(void)
{
ulong retcode;
ulong firmware;
bint err_counter = 0;


	vAHI_TimerFineGrainDIOControl(0x7F);   	// Входы-выходы таймеров не используются
	vAHI_DioSetDirection( 0,                  BIT_LED_GREEN_MAIN|OUT_LAMP );
	vAHI_DioSetPullup( 0, BIT_LED_GREEN_MAIN|OUT_LAMP|BIT_UART0_TX|BIT_UART1_TX|BIT_UART0_RX|INP_X );
	led_on( BIT_LED_GREEN_MAIN );
	led_on( OUT_LAMP );


	// Initialise the hardware API
    u32AHI_Init();

	set_timer_ms( &ledx_timer, 100 );

    init_console( CONSOLE_UART, CONSOLE_BAUD_RATE );


    // Setup interface to MAC
	firmware = app_inp_buf_init();

	hello_message();
	xprintf( "\tFirmware   :%8X\r", firmware );

	if( init_eeprom( TRUE ) )
		err_counter += POST_FATAL_ERR_CNT;

	//---------------------------------------------------------------------------
	// Initialise Beacon/Tag data
	//---------------------------------------------------------------------------
    // Initialise tick timer to give Inteval interrupt
	init_main_timer();
	// Инициализация данных фильтра запросов BI
	bif_init();
	//---------------------------------------------------------------------------
	init_unix_timer();


#if( USE_TOF )
	println( "\tInit TOF   -Ok" );
	tof_init();
#endif

	printst( "\tInit RADIO " );

    // Set up the MAC handles.
	// Must be called AFTER u32AppQApiInit()
    dd.mac_handler = pvAppApiGetMacHandle();
    dd.pib_handler = MAC_psPibGetHandle( dd.mac_handler );

    // Set Pan ID and short address in PIB (also sets match registers in hardware)
    MAC_vPibSetPanId( dd.mac_handler, PAN_ID );
    MAC_vPibSetShortAddr( dd.mac_handler, cfg.airid.id );

	retcode = eAppApiPlmeSet( PHY_PIB_ATTR_CURRENT_CHANNEL, cfg.airch.ch );
	if( retcode != PHY_ENUM_SUCCESS )
		xprintf( "SET:Ch failed(%2X)\r", retcode );
	else
		retcode = 0;

	// Set Out Power
	if( cfg.airch.out_power & (~CFG_TX_POWER_MASK ) )
		vAppApiSetHighPowerMode( APP_API_MODULE_HPM06, TRUE );

	if( eAppApiPlmeSet( PHY_PIB_ATTR_TX_POWER, cfg.airch.out_power & CFG_TX_POWER_MASK ) != PHY_ENUM_SUCCESS )
	{	println( "SET:Power failed" );
		++retcode;
	}

    // Enable receiver to be on when idle
    MAC_vPibSetRxOnWhenIdle( dd.mac_handler, TRUE, FALSE );

	if( retcode )
		xprintf( "-Fail(%2X)\r", retcode );
	else
	{   println( "-Ok" );
	}

	err_counter += retcode;

	if( err_counter )
	{
		led_mode( NUM_LED_GREEN_MAIN, LM_FLASH_2 );
		println( "Fail" );
	}
	else
	{
		led_mode( NUM_LED_GREEN_MAIN, LM_OFF );
		led_mode( NUM_LED_RED,   LM_OFF );
		println( "Done" );
	}

}

//---------------------------------------------------------------------------
// Check event queues and process and items found
//---------------------------------------------------------------------------
//static void process_queues(void)
void process_queues(void)
{
    // Check for anything on the MCPS upward queue
	for( ; ; )
    {   MAC_McpsDcfmInd_s *buf = mcps_buf_check();
		if( buf )
    	{   process_mcps_input( buf );
			if( sw_chk( STW_GET_OVERLOAD ) )
				sw_clr( STW_GET_OVERLOAD );
			mcps_buf_free( buf );
    	}
		else
		{	if( sw_chk( STW_GET_OVERLOAD ) )
			{ 	sw_set( STW_RESET_REQUEST );
			  	println( "QUE:Get overload fatal" );
			}
			break;
		}
    }
    // Check for anything on the MLME upward queue
	for( ; ; )
    {   MAC_MlmeDcfmInd_s *buf = mlme_buf_check();
        if( buf )
        {  	process_mlme_input( buf );
            mlme_buf_free( buf );
        }
		else
			break;
    }
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
static void process_mcps_input( MAC_McpsDcfmInd_s *mcps )
{
    // Only handle incoming data events one device has been started as a coordinator
	if( dd.state >= STATE_COORDINATOR_STARTED )
    {
        switch( mcps->u8Type )
        {

        case MAC_MCPS_IND_DATA:  // Incoming data frame
            rx_data_parser( (rx_frame_s *)mcps );
            break;

        case MAC_MCPS_DCFM_DATA: // Incoming acknowledgement or ack timeout
			handle_mcps_confirm( (data_confirm_s *)mcps );
            break;

        default:
			xprintf( "CPS:Type %2X\r", mcps->u8Type );
            break;
        }
    }
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void hello_message(void)
{
#if(   DEV_TAGXX == 9  )
	xprintf( "\n\n\rTOF Checker"
#elif( DEV_TAGXX == 1 )
	xprintf( "\n\n\rDEV Board TOF Checker"
#endif
			 " V%2u.%2u, %s %s, by I.Zalts\r\n",
						(BYTE)SYSVERSION, (BYTE)SUBVERSION,
						__DATE__, __TIME__ );
}


//---------------------------------------------------------------------------
//	функции для бутлодера
//---------------------------------------------------------------------------


int CheckFWPresent(void)	// проверка наличия прошивки
{
  // если прошивка на месте (первые 4 байт), то продолжаем проверки
  if (*(__IO uint32_t *)FIRMWARE_ADDRESS != FIRMWARE_EMPTY) {
    // если CRC и длина у прошивки отсутствуют, то считаем, что прошивка была залита через программатор
    if ((*(__IO uint32_t *)(FIRMWARE_ADDRESS + FIRMWARE_CRC_OFFSET) == FIRMWARE_EMPTY) &&
        (*(__IO uint32_t *)(FIRMWARE_ADDRESS + FIRMWARE_SIZE_OFFSET) == FIRMWARE_EMPTY)) {
      return 1;
    } else {
      // если CRC правильная
      if (CheckCRC())
        return 1;
    }
  }
  // прошивка отсутствует
  return 0;
}
