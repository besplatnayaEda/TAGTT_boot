
#include "jnx_common.h"

#include "jnx_console.h"
#include "tof.h"

#if( USE_TOF )


#if( USE_TOF > 1 )

bint ttl_process_tof( bint data_size );
bint ttl_start_tof( int offset );


//---------------------------------------------------------------------------
tof_data_s	tof;

void vTofCallback( eTofReturn status );

//---------------------------------------------------------------------------
// TOF
// RSSI to Distance (cm) lookup table. Generated from formula in JN-UG-3063
const ulong rssi_distance[] = {
		502377, 447744, 399052, 355656, 316979, 282508, 251785, 224404,
	    200000, 178250, 158866, 141589, 126191, 112468, 100237, 89337,
		79621,  70963,  63246,  56368,  50238,  44774,  39905,  35566,
		31698,  28251,  25179,  22440,  20000,  17825,  15887,  14159,
		12619,  11247,  10024,  8934,   7962,   7096,   6325,   5637,
        5024,   4477,   3991,   3557,   3170,   2825,   2518,   2244,
        2000,   1783,   1589,   1416,   1262,   1125,   1002,   893,
        796,    710,    632,    564,    502,    448,    399,    356,
        317,    283,    252,    224,    200,    178,    159,    142,
        126,    112,    100,    89,     80,     71,     63,     56,
        50,     45,     40,     36,     32,     28,     25,     22,
        20,     18,     16,     14,     13,     11,     10,     9,
        8,      7,      6,      6,      5,      4,      4,      4,
        3,      3,      3,      2,      2 };




//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint ttl_start_tof( bint idx )
{
	sw_clr( STW_TOF_DONE );

	tof.req_n = cfg.mode.tof_nnn;

	if( tof.req_n > TOF_MAX_REQUESTS )
	{  	println( "TOF:Invalid NN" );
		return( 1 );
	}


	if( bAppApiGetTof( &tof.info[idx], &tof.addr, tof.req_n, tof.direction, vTofCallback ) )
 	{	//dprintf( DL_TOF, "TOF:%5u started D:%1X\r", tof.addr.uAddr.u16Short, tof.direction );
		sw_set( STW_TOF_PROGRESS );
		return( 0 );
	}
   	xprintf( "TOF:%5u Failed to start\r", tof.addr.uAddr.u16Short  );
	return( 1 );
}



//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void tof_tester(void)
{
	tof.rerr_cnt = 0;
	do{

	xprintf( "TTL:%5u started\r", tof.addr.uAddr.u16Short );

	switch( cfg.mode.tof_mod & (BIT1|BIT0) )
	{
	case 0:
		tof.direction = 0;
		tof.phase     = 0;
		break;

	case 1:
		tof.direction = 1;
		tof.phase     = 0;
		break;

	case 2:
		tof.direction = 0;
		tof.phase     = 1;
		break;

	default:

		println( "TOF:Invalid MODE" );

		sw_clr( STW_TOF_TESTER|STW_TOF_PROGRESS|STW_TOF_DONE );

		return;


	}

	ttl_start_tof( 0 );

	while( sw_chk( STW_TOF_DONE ) == 0 );


	if( tof.phase == 1  )
	{   tof.direction = 1;

		ttl_start_tof( tof.req_n );

		while( sw_chk( STW_TOF_DONE ) == 0 );

		ttl_process_tof( tof.req_n * 2 );

		tof.direction = 1;

	}
	else
		ttl_process_tof( tof.req_n );

	vAHI_WatchdogRestart();

	}
	while( tof.loops-- );


	sw_clr( STW_TOF_TESTER|STW_TOF_PROGRESS|STW_TOF_DONE );

}



//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint ttl_process_tof( bint size )
{

int error_cnt = 0;

    for( bint n = 0; n < size; n++ )
    {	// Only include successful readings
        if( tof.info[n].u8Status == MAC_TOF_STATUS_SUCCESS )
        {
			xprintf( "TOR:%8X L:%2X R:%2X TS:%8X\r",
					tof.info[n].s32Tof,
        		    tof.info[n].s8LocalRSSI,
        		    tof.info[n].s8RemoteRSSI, tof.info[n].u32Timestamp );

			tof.rerr_cnt = 0;
      	}
        else
        {   error_cnt++;
          	xprintf( "TOF:Error:%2X\r",
        	        tof.info[n].u8Status );
        }
		if( error_cnt == size )
		{
			if( ++tof.rerr_cnt > 10 )
			{
	          	println( "TOF:Fatal Error - STOP" );
				tof.loops = 0;
				return( 1 );
			}
		}


    }
	return( 0 );
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint process_tof_double( bint show )
{
ulong dist_tof;
ulong dist_rss;

    for( bint n = 0; n < tof.req_n; n++ )
    {	// Only include successful readings
        if( tof.info[n].u8Status == MAC_TOF_STATUS_SUCCESS )
        {

//tof.info[n].s32Tof       = 1000000;
//tof.info[n].s8LocalRSSI  = 50;
//tof.info[n].s8RemoteRSSI = 60;

			if( tof.info[n].s32Tof < 0 )
			{	tof.summ_tof += ( tof.info[n].s32Tof * 3 - 500 )/1000;
				dist_tof = 0;
			}
			else
			{   dist_tof =    (((tof.info[n].s32Tof) * 3 + 500 )/1000 );  // В десятых метра
				tof.summ_tof += dist_tof;
			}

			switch( (cfg.mode.tof_mod & (BIT3|BIT2)) >> 2 )
			{
			case 0:
				tof.summ_rss += rssi_distance[tof.info[n].s8RemoteRSSI];
				break;
			case 1:
				tof.summ_rss += rssi_distance[tof.info[n].s8LocalRSSI ];
				break;
			case 2:
				tof.summ_rss += rssi_distance[tof.info[n].s8RemoteRSSI];
				tof.summ_rss += rssi_distance[tof.info[n].s8LocalRSSI ];
			}

			if( show )
			{   if( cfg.mode.tof_mod & BIT4 )
				{
				xprintf( "TOR:%8X L:%2X R:%2X TS:%8X\r",
						tof.info[n].s32Tof,
        		        tof.info[n].s8LocalRSSI,
        		        tof.info[n].s8RemoteRSSI, tof.info[n].u32Timestamp );
				}
				else
				{
        		xprintf( "TOF:%3u.%1um L:%5u R:%5u\r",
						dist_tof/10, dist_tof%10,
        		        rssi_distance[tof.info[n].s8LocalRSSI]/10,
        		        rssi_distance[tof.info[n].s8RemoteRSSI]/10 );
				}
			}
        }
        else
        {   tof.rerr_cnt++;
			if( show )
			{
          	xprintf( "TOF:Error:%2X\r",
        	        tof.info[n].u8Status );
			}
        }
    }


	if( tof.phase )
	{   // Второй/последний проход
		if( ((cfg.mode.tof_mod & (BIT1|BIT0))) == 2 )
		{	// Два прохода и значений вдвое больше
			tof.req_n = tof.req_n<<1;
		}


		if( tof.req_n <= tof.rerr_cnt )
			goto fatal_error;


		if( tof.summ_tof < 0 )
			tof.summ_tof = 0;

		dist_tof = tof.summ_tof/(tof.req_n - tof.rerr_cnt);
		dist_rss = tof.summ_rss/(tof.req_n - tof.rerr_cnt);

		if( ((cfg.mode.tof_mod & (BIT3|BIT2))>>2) == 2 )
			dist_rss = (dist_rss + 10 )/20;
		else
			dist_rss = (dist_rss + 5  )/10;

		if( show )
		{
    		xprintf( "DIS:%3u.%1um RSSI:%3u.%1um\r",
    	    	dist_tof/10, dist_tof%10,
    	    	dist_rss/10, dist_rss%10	 );
		}

//		if( dist_rss > TOF_MAX_RSSI )
//			dist_rss = TOF_MAX_RSSI;

//		if( dist_tof > TOF_MAX )
//			dist_tof = TOF_MAX;


//		tof.dist 	  = dist_tof;
//		tof.dist_rssi = dist_rss;

//		tof.sdev  = 0; 			// Пока среднеквадратичное не считаем

		tof.direction = 0;
		tof.phase     = 0;
		return( 2 );

	}
	else
	{	tof.direction = 1;
		tof.phase     = 1;
		// Запускаем повторное инверсное
		tof_start( 0, 1 );
		return( 1 );
	}

fatal_error:
	// Фатальная ошибка
	if( show )
    	println( "\rTOF:Out" );

//	tof.distance_full = 0xFFFFFFFFUL;

	tof.phase     = 0;
	tof.direction = 0;
	return( 0 );

}

//---------------------------------------------------------------------------
// This function is passed to bAppApiGetTof. Function is called when the tof
// readings have been completed and stored in asTofData.
//---------------------------------------------------------------------------
void vTofCallback( eTofReturn status )
{
	if( status == TOF_SUCCESS )
	{   if( sw_chk( STW_TOF_PROGRESS ) )
			sw_set_isr( STW_TOF_DONE );
	}
	else
	{  	sw_set_isr( STW_TOF_DONE|STW_TOF_FAIL );
	   	dprintf( DL_TOF, "TOF:Error:%2X\r", status );
	}
}


/*
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void tof_ii_start( ushort addr, BYTE flag )
{
	// Делаем через установку запроса
	iid.tid_addr = tof.addr.uAddr.u16Short = addr;
	iid.ext 	 = flag;



	if( iid.ext & TII_EXT_REQ_DISTANCE )
	{

		tof.req_n 				= cfg.mode.tof_nnn;
//		tof.direction 			= cfg.mode.tof_mod&BIT0;
// 		tof.addr.uAddr.u16Short = iid.tid_addr;

		if( tof.req_n > TOF_MAX_REQUESTS )
		{  	println( "TOF:Invalid NN" );
			return;
		}

		switch( cfg.mode.tof_mod & (BIT0|BIT1) )
		{
		case 0:
			// Однократное прямое измерение
		case 1:
			// Однократное инверсное измерение
			tof.direction = cfg.mode.tof_mod & BIT0;
			// Сброс суматоров
			tof.summ_rss = 0;
			tof.summ_tof = 0;
			tof.rerr_cnt = 0;
			break;

		case 2:
			// Двойное измерение - начинаем с прямого
//			if( phase )
//				tof.direction = 1;
//			else
//			{
				tof.direction = 0;
				// Сброс суматоров
				tof.summ_rss = 0;
				tof.summ_tof = 0;
				tof.rerr_cnt = 0;
//			}
			break;

		default:
			// Ошибка
		   	println( "TOF:Invalid Mode" );
			return;

		}


#if( DEV_ISIB3 && ( USE_SEGMENT_MODE == 1 ) )
		// Таймер по адресу с целью разрулить конфликты по TOF
		// Для ISBRF таймер не взводится, ибо он главный
		set_timer_ts( &slot_timer, TOF_COMMAND_TIMESLOTS );

#endif
		if( iid.ext & TII_EXT_REQ_DATA )
		{	iid.req_counter = 1;
			bs_set( BSF_MODE_II_REQUEST );
		}

		bs_set( BSF_MODE_TOF_REQUEST );
	}
	else
	{
#if( DEV_ISIB2 )
#else
		iid.req_counter = II_COMMAND_RETRY_COUNTER;
#endif


		// В случае самостоятельного тактирования запуск этого таймера
		// сбрасывает большой период ожидания посылки BI
//		( &slot_timer, II_COMMAND_TIMESLOTS );
		set_timer_ts( &slot_timer, II_COMMAND_TIMESLOTS );

		bs_set( BSF_MODE_II_REQUEST );
	}
}
*/


#if( DEV_ISIB3|DEV_ISBRF )
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint tof_request(void)
{
	vAppApiTofSetCalloffset( cfg.tof_offset );
//	vAppApiTofSetCalloffset( 0 );

	if( bAppApiGetTof( &tof.info[0], &tof.addr, tof.req_n, tof.direction, vTofCallback ) )
 	{	dprintf( DL_TOF, "TOF:%5u started\r", tof.addr.uAddr.u16Short );
		sw_set( STW_TOF_PROGRESS );
		sw_clr( STW_TOF_FAIL );
		return( 0 );
	}
	sw_set( STW_TOF_FAIL );
   	dprintf( DL_TOF, "TOF:%5u Failed to start\r", tof.addr.uAddr.u16Short );
	return( 1 );
}
#endif


//#if( DEV_IVT24 || DEV_ISBRF || DEV_ISIB3 )
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint tof_start_cmd( ushort addr, BYTE tof_mode )
{
	cfg.mode.tof_mod = tof_mode;
//	cfg.mode.tof_nnn;

	tof_start( addr, 0 );

	return( 1 );

}

//#endif

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
bint tof_start( ushort addr, bint phase )
{
	if( addr )
	{
//		iid.tid_addr  = tof.addr.uAddr.u16Short = addr;
		tof.addr.uAddr.u16Short = addr;
//		iid.tid_type      = 0; //TAG_TYPE_IPT24;
//		iid.tid_ext_ident = 0x00;
	}

	tof.req_n = cfg.mode.tof_nnn;

	if( tof.req_n > TOF_MAX_REQUESTS )
	{  	println( "TOF:Invalid NN" );
		return( 1 );
	}

	switch( cfg.mode.tof_mod & (BIT0|BIT1) )
	{
	case 0:
		// Однократное прямое измерение
	case 1:
		// Однократное инверсное измерение на начальной фазе
		tof.direction = cfg.mode.tof_mod & BIT0;
		// Сброс суматоров
		tof.summ_rss = tof.summ_tof = tof.rerr_cnt = 0;
		tof.phase    = 1;
		break;

	case 2:
		// Двойное измерение - начинаем с прямого
		if( phase )
		{	tof.direction = 1;
			tof.phase     = 1;
		}
		else
		{	tof.direction = 0;
			tof.phase     = 0;

			// Сброс суматоров
			tof.summ_rss = tof.summ_tof = tof.rerr_cnt = 0;
		}
		break;

	default:
		// Ошибка
	   	println( "TOF:Invalid Mode" );
		return( 1 );

	}

	vAppApiTofSetCalloffset( cfg.tof_offset );

	if( bAppApiGetTof( &tof.info[0], &tof.addr, tof.req_n, tof.direction, vTofCallback ) )
 	{	dprintf( DL_TOF, "TOF:%5u started D:%1X\r", tof.addr.uAddr.u16Short, tof.direction );
		sw_set( STW_TOF_PROGRESS );
		sw_clr( STW_TOF_FAIL );
		return( 0 );
	}
	sw_set( STW_TOF_FAIL );
   	dprintf( DL_TOF, "TOF:%5u Failed to start\r", tof.addr.uAddr.u16Short  );
	return( 1 );
}

#endif

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void tof_init(void)
{

#if( USE_TOF > 1 )
	bint n;
    for( n = 0; n < TOF_MAX_REQUESTS*2; n++ )
    {   tof.info[n].s32Tof       = 0;
        tof.info[n].s8LocalRSSI  = 0;
        tof.info[n].u8LocalSQI   = 0;
        tof.info[n].s8RemoteRSSI = 0;
        tof.info[n].u8RemoteSQI  = 0;
        tof.info[n].u32Timestamp = 0;
        tof.info[n].u8Status     = 0;
    }
    tof.addr.u8AddrMode     = 2;
    tof.addr.u16PanId       = PAN_ID;
    tof.addr.uAddr.u16Short = TAG_AIR_DEFAULT_ID;
#endif

	vAppApiTofInit( TRUE );
}

#endif
