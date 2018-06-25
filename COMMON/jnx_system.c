
#include "jnx_common.h"
#include "jnx_console.h"
#include "jnx_system.h"



static void unix_timer_call( uint32 dev, uint32 map );

#define TSTAT_TIM_RUN	(BIT0)

volatile ulong unix_timer_cnt      __attribute__ ((section ("UNIX_TIME")));
volatile ulong unix_timer_cnt_sign __attribute__ ((section ("UNIX_TIME")));
ulong unix_timer;

#if( DEV_ISIB3 || DEV_BRF24 || DEV_ISBRF || DEV_IVT24 )
sys_timer_s slot_timer;
#endif

#if( DEV_ISIB3 || DEV_ISBRF )
#include "jnx_uart_slip.h"
#endif

#if( DEV_TAGXX || DEV_IVT24 )
sys_timer_s regs_timer;
sys_timer_s ccir_timer;
#endif

sys_timer_s ledx_timer;

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void set_timer( sys_timer_s *tim, ulong value )
{
	tim->value = value;
	tim->limit = u32AHI_TickTimerRead() + value;
	tim->tstat |= TSTAT_TIM_RUN;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void set_timer_go( sys_timer_s *tim, ulong value )
{
	tim->value = value;
	tim->limit = u32AHI_TickTimerRead() - 1;
	tim->tstat |= TSTAT_TIM_RUN;
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void chg_timer( sys_timer_s *tim, ulong value )
{
	tim->value = value;
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void rst_timer( sys_timer_s *tim )
{
	tim->limit = u32AHI_TickTimerRead() + tim->value;
	tim->tstat |= TSTAT_TIM_RUN;
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void stp_timer( sys_timer_s *tim )
{
	tim->tstat &= (~TSTAT_TIM_RUN);
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint chk_timer( sys_timer_s *tim )
{
	if( tim->tstat & TSTAT_TIM_RUN )
	{	if( tim->limit - u32AHI_TickTimerRead() >= tim->value )
			return( TRUE );
	}
	return( FALSE );
}

//---------------------------------------------------------------------------
//  Активизировать таймер немедленно
//---------------------------------------------------------------------------
void res_timer( sys_timer_s *tim )
{
	tim->limit = u32AHI_TickTimerRead() + (tim->value<<1);
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint chk_timer_restart( sys_timer_s *tim )
{
	if( tim->tstat & TSTAT_TIM_RUN )
	{	ulong cnt = u32AHI_TickTimerRead();
		if( tim->limit - cnt >= tim->value )
		{   tim->limit = cnt +  tim->value;
			return( TRUE );
		}
	}
	return( FALSE );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint chk_timer_stop( sys_timer_s *tim )
{
	if( tim->tstat & TSTAT_TIM_RUN )
	{	if( tim->limit - u32AHI_TickTimerRead() >= tim->value )
		{   tim->tstat &= (~TSTAT_TIM_RUN);
			return( TRUE );
		}
	}
	return( FALSE );
}



//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint init_main_timer(void)
{
//    vAHI_TickTimerConfigure( E_AHI_TICK_TIMER_DISABLE );
//    vAHI_TickTimerWrite( 0 );
//    vAHI_TickTimerInit( main_timer_isr );
//    vAHI_TickTimerInterval( TICK_PERIOD_COUNT );
//    vAHI_TickTimerConfigure( E_AHI_TICK_TIMER_RESTART );
//    vAHI_TickTimerIntEnable( TRUE );


#if( DEV_TAGXX || DEV_IVT24 )
	unix_timer_cnt = 60*60; 		// Для нормальной раболты фильтра нельзя начинать с 0. Пусть будет час
#else
	// По умолчанию рабочий режим
	bs_set( BSF_RESET_FLAG|BSF_MODE_WORK );
	if( unix_timer_cnt != (~unix_timer_cnt_sign) )
	{	unix_timer_cnt = 60*60; 		// Для нормальной раболты фильтра нельзя начинать с 0. Пусть будет час
		bs_set( BSF_COLD_START_FLAG );
	}
#endif

	unix_timer = unix_timer_cnt;
    vAHI_TickTimerConfigure( E_AHI_TICK_TIMER_DISABLE );
    vAHI_TickTimerConfigure( E_AHI_TICK_TIMER_CONT );
	return( 0 );
}

/*
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
static void main_timer_isr( uint32 dev, uint32 map )
{
ARGSUSED( dev );
ARGSUSED( map );

	++main_timer_cnt;
}
*/

//---------------------------------------------------------------------------
#define UTIME_PRESCALER 	(8)      // 2^8 = 256
#define UTIME_DIVIDER 		(((SYS_FREQUENCY)/256))

void utime_callback( ulong dev, ulong map );
//---------------------------------------------------------------------------
bint init_unix_timer(void)
{
	// Самое низкоприоритетное прерывание
	vAHI_InterruptSetPriority( MICRO_ISR_MASK_TMR4, UNIX_TIMER_INT_PRIORITY );
	// Инициализация секундного таймера
	vAHI_TimerDIOControl( E_AHI_TIMER_4, FALSE );
	vAHI_Timer4RegisterCallback( unix_timer_call );
	vAHI_TimerEnable( E_AHI_TIMER_4, UTIME_PRESCALER, FALSE, TRUE, FALSE );
	vAHI_TimerConfigureOutputs( E_AHI_TIMER_4, FALSE, TRUE );
	vAHI_TimerSetLocation( E_AHI_TIMER_4, TRUE, FALSE );

	vAHI_TimerStartRepeat( E_AHI_TIMER_4, 10, UTIME_DIVIDER );

	return( 0 );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
static void unix_timer_call( ulong dev, ulong map )
{
ARGSUSED( dev );
ARGSUSED( map );

/*
    if( dev != E_AHI_DEVICE_TIMER4 )
    {  	xprintf( "4XX:(%1X)TIME IQ %2u:%2u UT unknown MAP:%8X\r", get_us_state(), ud.irx, ud.itx, map );
		return;
	}
*/

	disable();

	unix_timer_cnt++;
	unix_timer_cnt_sign = ~unix_timer_cnt;

#if( DEV_ISIB3 && ( USE_UART_MODE == MODE_UART_485 ) )
	if( chk_us_state( US_RX ) )
	{   int state = u32AHI_DioReadInput();
		if( state &( RXE_422_N | TXE_422  ) )
		{
			xprintf( "4XX:[%1X]--[%1X] EN:%1X %2u:%2u Crash %s\r", get_us_state(), get_us_state_prev(), (state&(RXE_422_N|TXE_422))>>16, ud.irx, ud.itx, get_time_string( unix_timer ) );
			set_us_state( US_TX_CRASH );

		}
	}
#endif

}


//---------------------------------------------------------------------------
//	Fills the time structure tm by translating the unixtime
//
static const uchar _Days[12] = {
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };

static const ushort _monthDay [13] = {
   0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};
struct  tm timex = {
   0, 0, 0, 1, 0, 107, 4, 0, 0
}; 	// 1 jan 2007
//---------------------------------------------------------------------------
#define TIME_STR_SIZE (24)
static char ascii_time[TIME_STR_SIZE];

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
struct tm *tmtime( ulong utime )
{
int i;
int cumdays;

    timex.tm_sec = (int)(utime % 60);
    utime /= 60;                            // Time in minutes
    timex.tm_min = (int)(utime % 60);
    utime /= 60;                            // Time in hours
    i = (unsigned)(utime / (1461L*24L));   	// Number of 4 year blocks
    timex.tm_year = (i << 2);
    timex.tm_year += 70;
    cumdays = 1461*i;
    utime %= 1461L*24L;        				// Hours since end of last 4 year block

    for( ; ; )
   	{   uint hpery = 365*24;
        if( ( timex.tm_year & 3 ) == 0 )
            hpery += 24;
        if( utime < hpery )
            break;
        cumdays += hpery/24;
        timex.tm_year++;
        utime -= hpery;
    }   									// At end, utime is number of hours into current year

//    if( dst && __isDST( (int)(utime % 24), (int)(utime / 24), 0, timex.tm_year-70 ) )
//   	{   utime++;
//        timex.tm_isdst = 1;
//    }
//    else
        timex.tm_isdst = 0;

	timex.tm_hour = (int)( utime % 24 );
    utime /= 24;             // Time in days
    timex.tm_yday = (int)utime;
    cumdays += (int)utime + 4;
    timex.tm_wday = cumdays % 7;
    utime++;

    if( ( timex.tm_year & 3 ) == 0 )
    {   if( utime > 60 )
        	utime--;
        else
        {   if( utime == 60 )
            {   timex.tm_mon = 1;
                timex.tm_mday = 29;
                return( &timex );
            }
		}
	}
    for( timex.tm_mon = 0; _Days[timex.tm_mon] < utime; timex.tm_mon++ )
        utime -= _Days[timex.tm_mon];

	timex.tm_mday = (int)(utime);

	return( &timex );
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
char *get_time_string( ulong utime )
{
#if( DEV_TAGXX || DEV_IVT24 || DEV_ISBRF )
	strftime( ascii_time, TIME_STR_SIZE, "%M:%S", tmtime( ntohs(utime) ) );
#else
	strftime( ascii_time, TIME_STR_SIZE, "%d.%b %H:%M:%S", tmtime( ntohs( utime +2*3600) ) );
#endif
	return( ascii_time );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void reset( bint flags )
{
	if( flags )
	{   vAHI_WatchdogStop();
		vAHI_WatchdogStart( 0 );  	//
		for( ; ; );
	}
	else
		vAHI_SwReset();

}

//---------------------------------------------------------------------------
volatile bint delay_cnt;
//
//---------------------------------------------------------------------------
void dummy_delay( bint x )
{
	for( delay_cnt = 0; delay_cnt < x; delay_cnt++ );
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void flash_invalidate(void)
{
#define REC_SIZE	(16)
BYTE wrbuff[REC_SIZE];
	bAHI_FlashInit( E_FL_CHIP_INTERNAL, NULL );
	bAHI_FullFlashRead(    0, REC_SIZE, (uint8 *)&wrbuff );
	wrbuff[13]  = 0;
	bAHI_FullFlashProgram( 0, REC_SIZE, (uint8 *)&wrbuff );
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint check_protection(void)
{
ulong value = *(ulong *)0x01001510;

	switch( (value >> 4) & 0x3 )
	{
	case 0x1:
		return( 1 );
	case 0x3:
		return( 0 );
	}
	return( 2 );
}

#if( DEV_TAGXX )
#else
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
int io_control( BYTE ch, io_control_u cmd_io )
{
	switch( ch )
	{
	case 0:
	case 1:

		if( cmd_io.cfg_set_flag )
		{
			cfg.iocfg[ch].io_cfg = cmd_io.io_control & 0xF0;

			iod.ch[ch].io_state &= 0x0F;
			iod.ch[ch].io_state |= cmd_io.io_control & 0xF0;

			sw_set( STW_CFG_SAVE_REQUEST );
		}

		if( cmd_io.o_state_set_flag )
		{	// Управление выходом
			if( ch == 0 )
			{	if( cmd_io.o_state )
					pin_lo( OUT_1 );
				else
					pin_hi( OUT_1 );
			}
			else
			{	if( cmd_io.o_state )
					pin_lo( OUT_2 );
				else
					pin_hi( OUT_2 );
			}

			iod.ch[ch].o_state = cmd_io.o_state;
			iod.ch[ch].o_ctrl = TRUE;
		}

		if( cmd_io.event_store_flag )
			sw_set( STW_IO_EVENT_STORE );

		return( 0 );


	default:
		return( 1 );
	}
}
#endif


