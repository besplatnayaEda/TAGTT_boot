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

// запуск прошивки
void FirmwareStart(void);

// проверка CRC уже прошитой программы
int CheckCRC(void);

// загрузка hex через терминал
int hex_loader(void);


// проверка наличия прошивки
int FWPresent;


//---------------------------------------------------------------------------

void AppColdStart(void)
{
    // Disable watchdog if enabled by default
    vAHI_WatchdogStop();

    init_tag_system();


    // проверяем наличие прошивки после инициализации перефирии
    FWPresent = CheckFWPresent();

    // если в RAM сигнатуры нет, то запускаем прошивку (если она там есть)
    // иначе, если есть сигнатура - ждём прошивки
    //if (*(volatile uint32_t *)RAM_SIGNATURE_ADDRESS != RAM_SIGNATURE) {		// при выходе из прошивки в бутлоадер, нужно как-то отметить это в оперативке
      // если прошивка есть
      if (FWPresent)
        // запуск прошивки
        FirmwareStart();
    //}


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
#elif( DEV_TAGXX == 10 )
	xprintf( "\n\n\rBootloader"
#endif
			 " V%2u.%2u, %s %s, by K.Artemev\r\n",
						(BYTE)SYSVERSION, (BYTE)SUBVERSION,
						__DATE__, __TIME__ );
}


//---------------------------------------------------------------------------
//	функции для бутлодера
//---------------------------------------------------------------------------


int CheckFWPresent(void)	// проверка наличия прошивки
{
  /*// если прошивка на месте (первые 4 байт), то продолжаем проверки
  if (*(volatile uint32_t *)FIRMWARE_ADDRESS != FIRMWARE_EMPTY) {
    // если CRC и длина у прошивки отсутствуют, то считаем, что прошивка была залита через программатор
    if ((*(volatile uint32_t *)(FIRMWARE_ADDRESS + FIRMWARE_CRC_OFFSET) == FIRMWARE_EMPTY) &&
        (*(volatile uint32_t *)(FIRMWARE_ADDRESS + FIRMWARE_SIZE_OFFSET) == FIRMWARE_EMPTY)) {
      return 1;
    } else {
      // если CRC правильная
      //if (CheckCRC())			// должна быть какая-то проверка crc
        return 1;
    }
  }*/		// пока рассмотрим вариант, что прошивка или есть или нет
  // прошивка отсутствует (0)
  return 0;
}


int CheckCRC(void)
{
  uint32_t crc;
  uint32_t size;

  size = *(volatile uint32_t *)(FIRMWARE_ADDRESS + FIRMWARE_SIZE_OFFSET);
  // посчитать CRC
  crc = crc32(0xFFFFFFFF,(const void *)FIRMWARE_ADDRESS, size);

  // сравниваем CRC
  return (crc == *(volatile uint32_t *)(FIRMWARE_ADDRESS + FIRMWARE_CRC_OFFSET));
}


void FirmwareStart(void)
{

  typedef void (*pFunction)(void);
  pFunction Jump_To_Application;
  uint32_t JumpAddress;


  // деинициализация модулей

  vAHI_UartDisable( CONSOLE_UART );		// откл уарт
  vAHI_TimerDisable( E_AHI_TIMER_4 );	// откл таймер 4



  // готовимся к переходу на пользовательскую программу
  // отключить все прерывания
  MICRO_DISABLE_INTERRUPTS();
  // перенести таблицу векторов

  (*(volatile uint32_t *)(VECTOR_TABLE)) = (FIRMWARE_ADDRESS);				// неизветсно, как будет работать, скорее всего никак


  // адрес перехода
  JumpAddress = *(volatile uint32_t*)(FIRMWARE_ADDRESS + 4);
  Jump_To_Application = (pFunction)JumpAddress;
  // установить стек

  (*(volatile uint32 *)(STACK_ADRESS)) = (*(volatile uint32_t*)FIRMWARE_ADDRESS);	// но таблицу векторов и стек необходимо перенести

  // включить все прерывания
  MICRO_ENABLE_INTERRUPTS();
  // переход
  Jump_To_Application();


/*
  // инит TFT модуля
  TFT_Init();
	// Включение LCD
	HAL_GPIO_WritePin(PWM1_GPIO_Port, PWM1_Pin, GPIO_PIN_SET);
  // очистка экрана
	TFT_clear();
  TFT_printf(0, 0, FONT_UM13x28, clBlack, clWhite, "Запуск прошивки!");

  // !!!!
  *(__IO uint32_t *)RAM_SIGNATURE_ADDRESS = RAM_SIGNATURE;
*/

  while (1) {}

}

// модифицируем функцию из ARM
int hex_loader()
{
ulong high_addr = 0;
ulong page_addr = 0;
ushort rec_addr;
BYTE rec_crc;
BYTE rec_size;
BYTE rec_type;
int nak_cnt;
BYTE *rxf_ptr;
BYTE *wpb_ptr = &page_buf[0];
int   wpb_cnt = 0;

	lpc_flash_init();		// инициализация flash

	for( ; ; )
	{   // Wait delimiter
		nak_cnt = 0;            // Attempts counter
		int ii = 80;
		while( get_byte() != ':' )
		{	if( !(ii--) )
		    {	send_byte( FRAME_CAN );
				return( 1 );
			}
		}
		if( status_word & STW_WATCHDOG )
		{   WDFEED = 0xAA;
			WDFEED = 0x55;
		}
		// Get the 8bit frame size
		rec_size  = ((int)a2h( get_byte() ))<<4;
		rec_size |= ((int)a2h( get_byte() ));
		rec_crc = rec_size;
		rec_addr  = ((int)a2h( get_byte() ))<<12;
		rec_addr |= ((int)a2h( get_byte() ))<<8;
		rec_crc += rec_addr>>8;
		rec_addr |= ((int)a2h( get_byte() ))<<4;
		rec_addr |= ((int)a2h( get_byte() ));
		rec_crc += (BYTE)(rec_addr&0x00FF);
		rec_type  = ((int)a2h( get_byte() ))<<4;
		rec_type |= ((int)a2h( get_byte() ));
		rec_crc += rec_type;

		// Receive a frame of data from communication interface and calculate its CRC
        for( rxf_ptr=rxf_buf, ii=rec_size+1; ii; ii-- )
		{   BYTE ch;
			ch  = a2h( get_byte() )<<4;
			ch |= a2h( get_byte() );
			*rxf_ptr++ = ch;
			rec_crc +=  ch;
		}


//xprintf( "\rF:%02X A:%06X Type:%02X S:%02X- ",rec_size, high_addr+rec_addr, rec_type, rec_crc );
//for( int ik=0; ik<= rec_size; ik++)
//	xprintf( "%02X", rxf_buf[ik] );
//xprintf("\r");

		// Summ is OK?
		if( rec_crc == 0 )
		{   //ptr = rxf_buf;

			switch( rec_type )
			{
				// Data Record (8-, 16-, 32-bit formats)
		case 0:
				memcpy_z( wpb_ptr, &rxf_buf[0], rec_size );
				if( wpb_cnt == 0 )
				{	page_addr = high_addr + rec_addr;
//xprintf( "Set Page Addr:%06X\r", page_addr );
				}
				wpb_ptr += rec_size;
				wpb_cnt += rec_size;
				if( wpb_cnt >=  PAGE_SIZE )
				{   wpb_ptr = &page_buf[0];
					wpb_cnt = 0;

//xprintf( "Flash Addr:%06X\r", page_addr );
					if( lpc_flash( page_addr, &page_buf[0], PAGE_SIZE, FALSE ) )
					{   send_byte( FRAME_CAN );
				    	return( 1 );
					}
				}

				break;

			// End of File Record (8-, 16-, 32-bit formats)
		case 1:
				if( wpb_cnt )
				{   // Fill tail
					memset_z( &page_buf[wpb_cnt], 0xFF, PAGE_SIZE - wpb_cnt );
					if( lpc_flash( page_addr, &page_buf[0], PAGE_SIZE, FALSE ) )
					{   send_byte( FRAME_CAN );
				    	return( 1 );
					}
				}
				send_byte( FRAME_COMPLETE );
				return( 0 );

			// Extended Segment Address Record (16- or 32-bit formats)
		case 2:
				high_addr = (rxf_buf[0] << 12) + (rxf_buf[1] << 4);
//xprintf( "High 02 Addr:%06X\r", high_addr );
				break;

			// Start Segment Address Record (16- or 32-bit formats)
		case 3:
//xprintf( "High 03 Addr:%06X\r", high_addr );
				break;

			// Extended Linear Address Record (32-bit format)
		case 4:
				high_addr = (rxf_buf[0] << 24) + (rxf_buf[1] << 16);
//xprintf( "High 04 Addr:%06X\r", high_addr );
				break;

			// Start Linear Address Record (32-bit format)
		case 5:
//xprintf( "High 05 Addr:%06X\r", high_addr );
				break;

			// Unknown type
		default:
    			send_byte( FRAME_CAN );
				return( 3 );
			}
			send_byte( FRAME_ACK );
		}
    	else
    	{
printst( "\rBad CRC\r");
			if( nak_cnt-- )
				send_byte( FRAME_NAK );
	     	else
    		{	send_byte( FRAME_CAN );
		     	return( 4 );
			}
		}
	}
}
