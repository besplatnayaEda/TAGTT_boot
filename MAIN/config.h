 //---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#ifndef  __CONFIG_H_INCLUDED
#define  __CONFIG_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

#include <jendefs.h>

// TAGXX = 3	IVT24 (�� ISIB3/RF24 )
// TAGXX = 1	�������� ��� �� development board
// TAGXX = 2	�������� ��� �� USB �������
// TAGXX = 5	�������� ��� �� RF24
// TAGXX = 6	SPT24
// TAGXX = 7	SPT24->IPT24_GAS
// TAGXX = 8	TESTER ATEST
// TAGXX = 9	TOF Tester
// TAGXX = 10	���-bootloader

#define DEV_TAGXX           (10)
#define USE_DEBUG_GLOBAL	(0)
#define USE_ZONE_MODE 		(0)

//---------------------------------------------------------------------------

#define SYSVERSION 			0
#define SUBVERSION	    	0

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
#define USE_AIR_DIAG		(0) 	// ���������� ������� ������� �����������
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
#define USE_TOF	   			(0) 	// ���������� TOF
#define USE_VLF	    		(0)
#define IOCH_NN			   	(0)     // ��� ���������� ������-�������
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


// ����� ����������
#define BOOT_ADDRESS 0x00080000				// JN-AN-1003	sheet 22
#define BOOT_SIZE 0x00018000				// ������ ���������� ������ ����� ��� ������ 96 �� (���� ������� ��������)
// ����� ��������
#define FIRMWARE_ADDRESS (BOOT_ADDRESS + BOOT_SIZE)

// ������ � ������ ���������������� ����� � ����������
#define BOOT_USER_FIELDS_OFFSET 0x01AC			// ???
#define BOOT_USER_FIELDS_SIZE 4
// �������� ������� (������������ ������ ����������) ������ ����������
#define BOOT_VERSION_OFFSET BOOT_USER_FIELDS_OFFSET

// ��������� � RAM
#define RAM_SIGNATURE 0xAA551122				// ������������, ��� ����������� �������������
#define RAM_SIGNATURE_ADDRESS 0x04002FFC			// ???
// ��������, ����� �������� ���
#define FIRMWARE_EMPTY 0xFFFFFFFF
// ������ � ������ ���������������� ����� � ��������
#define FIRMWARE_USER_FIELDS_OFFSET 0x01AC			// ???
#define FIRMWARE_USER_FIELDS_SIZE 12
// �������� ������� (������������ ������ ��������) ����� �������� � CRC
#define FIRMWARE_SIZE_OFFSET FIRMWARE_USER_FIELDS_OFFSET
#define FIRMWARE_CRC_OFFSET (FIRMWARE_USER_FIELDS_OFFSET + 4)
#define FIRMWARE_VERSION_OFFSET (FIRMWARE_USER_FIELDS_OFFSET + 8)
// ������ �����
#define INT_FLASH_END 0x000C0000
#define FLASH_SIZE (INT_FLASH_END - BOOT_ADDRESS)		// ��������� ������ �����

#define CRC_INIT_RESET 0xFFFFFFFF		// ��������� �������� crc
#define VECTOR_TABLE 0x00080038			// ������� ��������
#define	RAM_END 0x04008000				// ��������� RAM
#define STACK_SIZE 0x00000800			// ������ �����
#define	STACK_ADRESS ( RAM_END - STACK_SIZE)		// ����� �����

#define FRAME_ACK  0x06  // ACK
#define FRAME_NAK   0x15        // Negative ACK
#define FRAME_COMPLETE 0x05        // ENQ =
#define FRAME_CAN   0x18        // CANcel


#if defined __cplusplus
}
#endif

#endif  // __CONFIG_H_INCLUDED

