
//

#ifndef _JNX_FLASH_LED_H
#define _JNX_FLASH_LED_H

void led_process( bint counter );
void led_mode( bint led, bint mode );
bint led_mode_get( bint led );


#define MAX_LEDS	(2)


typedef struct led_data_s {
	ulong stat;
	BYTE  mode[MAX_LEDS];
} led_data_s;


typedef enum{
	LM_OFF = 0,
	LM_ON = 1,
	LM_BLINK_SLOW,
	LM_BLINK_NORMAL,
	LM_BLINK_FAST,
	LM_FLASH_1,
	LM_FLASH_2,
	LM_FLASH_3,
	LM_FLASH_1_OFF = 8,
	LM_FLASH_2_OFF = 9,
	LM_MAX
} led_status;


extern led_data_s ldat;


#endif
