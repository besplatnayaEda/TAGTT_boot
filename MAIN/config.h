 //---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#ifndef  __CONFIG_H_INCLUDED
#define  __CONFIG_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

#include <jendefs.h>

// TAGXX = 3	IVT24 (на ISIB3/RF24 )
// TAGXX = 1	Тестовый таг на development board
// TAGXX = 2	Тестовый таг нв USB свистке
// TAGXX = 5	Тестовый таг нв RF24
// TAGXX = 6	SPT24
// TAGXX = 7	SPT24->IPT24_GAS
// TAGXX = 8	TESTER ATEST
// TAGXX = 9	TOF Tester

#define DEV_TAGXX           (9)
#define USE_DEBUG_GLOBAL	(0)
#define USE_ZONE_MODE 		(0)

//---------------------------------------------------------------------------

#define SYSVERSION 			0
#define SUBVERSION	    	1

#define TIMER_FR_INT_PRIORITY 			(1)
#define UART_DATA_INT_PRIORITY 			(2)

#define TIME_REREG_DEFAULT	  			(2*60)
#define TIME_REREG_DEFAULT_LAMPROOM	  	(15*60)


#define USE_CONSOLE    		(1)

// Timer data
#define TICK_PERIOD_ms          (10UL)	 		// Timer period
#define TICK_PERIOD_COUNT       (16000UL * TICK_PERIOD_ms)

// Duration (ms) = 15.36ms x (2^ACTIVE_SCAN_DURATION + 1)
#define ACTIVE_SCAN_DURATION        (3)
// Duration (ms) = 15.36ms x (2^ENERGY_SCAN_DURATION + 1)
#define ENERGY_SCAN_DURATION        (3)



#define USE_BUTTON			(0)
#define USE_AIR_DIAG		(0) 	// Используем функции эфирной диагностики
#define USE_DATA_UART  		(0)
#define USE_POWER_SLEEP 	(0)

#define USE_ADD_TRAPLIST 	(0)
#define USE_SEGMENT_MODE 	(0)
#define DEV_ZC03            (0)
#define USE_BLOCKING_TRAP   (0)
#define USE_MODEM_MODE 		(0)


#define NUM_LED_GREEN_MAIN 		(0)
#define BIT_LED_GREEN_MAIN		BIT4
#define POS_LED_GREEN_MAIN		(4)

#define NUM_LED_RED		   		(1)    // Dummy
#define BIT_LED_RED				BIT5
#define POS_LED_RED				(5)


#define OUT_LAMP 			BIT12
#define INP_X				BIT13


#define led_on( x )  		pin_hi( x )
#define led_off( x ) 		pin_lo( x )

#define USE_LCD	    		(0)
#define USE_TOF	   			(0) 	// Используем TOF
#define USE_VLF	    		(0)
#define IOCH_NN			   	(0)     // Нет дискретных входов-выходов
#define USE_LCD_I2C         (0)

#define BIT_UART0_TX	  		BIT6
#define BIT_UART0_RX	  		BIT7
#define BIT_UART1_TX	  		BIT11


#define DEV_ISBRF	    	(0)
#define DEV_ISIB3	    	(0)
#define DEV_ISIB2 			(0)
#define DEV_ISPTX          	(0)
#define DEV_BRF24           (0)
#define DEV_IVT24           (0)
#define DEV_ISIB2_433 		(0)
#define USE_END_DEVICES     (0)

#define CFG_SIGNATURE       (0xBEEF1234)
#define DEV_TYPE       		(0x77)


#define UNIX_TIMER_INT_PRIORITY		(1)

#define TOF_REQUESTS_NNN       (8)



#if defined __cplusplus
}
#endif

#endif  // __CONFIG_H_INCLUDED

