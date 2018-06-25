
#include <string.h>

#include "jnx_common.h"

#include "crc32.h"

#define MAX_BIR_RETRY  (12)


static bint chk_beacons_list( rx_frame_s *f );
bif_data_s 	 bif; 			// BI Filter operation data
ushort ccir_dest_addr;


static void registration_timeout(void);
static void run_registration_timer( ulong interval );
static void inc_bi_counter( rx_frame_s *f );


//---------------------------------------------------------------------------
// ��������� ������� �������� BI.
// ����������, ���� ��� ������� ������, �� ��� ��������� �� � ������������
// ������
//---------------------------------------------------------------------------
static void inc_bi_counter( rx_frame_s *f )
{
// ��������� �������������� ������ - bif.idx_handle. C ���� �������� ����� � ������
int idx = bif.idx_handle;
	do{
		if( bif.rec[idx].addr )
		{	// ���� �������� ������.
			if( bif.rec[idx].addr == f->src.addr )
			{   // ����� ������ � ������� ��������.
				// ���������� ����������� ������� BI �������� ������� ������
				bif.rec[idx].cnt_bi++;
			}
		}
		// ����� ����� �� ��������
		if( (--idx) < 0 )
			idx = bif.idx_max;
	}
	while( idx != bif.idx_handle );
}

//---------------------------------------------------------------------------
// Superfluous BI requests filter.
// ��������� ��� ��������� BI �������. ������� ���������� ������� � �������
// � ������ ����� ������������� ��� ������ �������� ����� � ��������.
// ���������� ���������� �������� ��� ������� �� �������� ����� ������������� ACK.
// �����-�� ����������� ���������� ���������� ������� � ���� ��������� ����
//---------------------------------------------------------------------------
static bint chk_beacons_list( rx_frame_s *f )
{
// ��������� ������������� ������ - bif.idx_handle. C ���� �������� ����� � ������
int idx = bif.idx_handle;
bif.rec_cnt  = 0;
int idx_free = HANDLE_INVALID;

	do{
		if( bif.rec[idx].addr )
		{	// ���� �������� ������. ����������� ������� �������� �������.
			// ���� � �������� ������ ������� ����� �� ����� ������ � ������,
			// �� ������� ������� ����� ���������� �������� ���������� �������
			// � ������� � ����� ���� ����������� ��� �������
			bif.rec_cnt++;

			if( bif.rec[idx].addr == f->src.addr )
			{   // ����� ������ � ������� ��������.
				// ���������� ����������� ������� BI �������� �� ���������� ���������� � ������� ��������
				// ������ �����
				bif.rec[idx].cnt_bi++;

				if( bif.rec[idx].cnt_bir > MAX_BIR_RETRY )
				{   // ��� �� ��������� �� ������ ���� ����� ��� �������������.
					// ���������� ��������, �� �� ������ ����� �� ����������
xprintf( "DIS:%4X [%1X] [%1X]\r", bif.rec[idx].addr, idx, bif.rec[idx].state );
					bif.rec[idx].state = BIF_STATE_WAIT;
					bif.rec[idx].cnt_bir >>= 1;		// ��������� �������, �� ��������� "�����"
					return( bif.rec[idx].state );
				}

				switch( bif.rec[idx].state )
				{
					//-------------------------------------------------------
				case BIF_STATE_REQ:
					// ��������� nandle ��� �������
					bif.idx_handle = idx;
					// ����������� ������� ������� BIR �������
					bif.rec[idx].cnt_bir++;

if( chk_debug( DL_QUALITY ) )
{
xprintf( "Rep>%4X [%1X]\r", bif.rec[bif.idx_handle].addr, bif.idx_handle );
}

				}
				return( bif.rec[idx].state );
			}
		}
		else
		{	// ���������� �� ������ ������ ��������� ��������� � ������ ��������� �����
			// ��� ������ ������ ������, ����� �� ��������
			idx_free = idx;
		}
		// ����� ����� �� ��������
		if( (--idx) < 0 )
			idx = bif.idx_max;
	}
	while( idx != bif.idx_handle );

	//-----------------------------------------------------------------------
	// �� ������ � ������ - �������� ����� �����
	if( ( bif.rec_cnt <= bif.idx_max )&&( idx_free > HANDLE_INVALID ) )
	{	// ���� ��������� ����� � ������ - ���������.
		// �� ����� �� ��������. ������ ������ ��� ��������� ������� �������.
		// ��� ���-�� ����� � ����� �������� �������� ����� ��� ���

		idx = idx_free;


		bif.rec[idx].addr      = f->src.addr;			// ���������� ����� ������ ������.
		bif.rec[idx].cod_slots = f->bi.cod_slots;   	// ���������� ������ ��� ���������� � ����������
 		bif.rec[idx].features  = f->dev_features;

//#if( USE_BI_EXTENDED )
		// � BI ������� �������������� ����
		if( ( f->len == sizeof(bi_short_s) )||( f->bi.reg_timeout == 0 ) )
		{	// ����� ������������� �� ������
			// ������ ������ ���������������
			bif.rec[idx].reg_timeout = ( ( bif.rec[idx].features & DF_LAMP_ROOM ) ? TIME_REREG_DEFAULT_LAMPROOM : cfg.dev_config.time_reg );
		}
		else
			bif.rec[idx].reg_timeout = f->bi.reg_timeout * 5;   // ���������� ������ ���������������
//#endif

		bif.rec[idx].state = BIF_STATE_REQ;
		// ���������� ������������� �������� BI �������� ���������� �� ����� ������ � ������� BIR
		bif.rec[idx].cnt_bi  = 1;
		bif.rec[idx].cnt_bir = 0;

		return( BIF_STATE_WAIT );
	}
	else
	{	// ����� � ������ ��� - ������������

		return( BIF_STATE_WAIT );
	}
}


//---------------------------------------------------------------------------
sys_timer_s reg;
#define TSTAT_TIM_RUN	(BIT0)
// ���������� ������� �������������. � ���������� ������ ����������� �����
// ������ ����� �� ������ ������� � ����������� ������� �������������
//---------------------------------------------------------------------------
void chk_registration(void)
{
	if( reg.tstat & TSTAT_TIM_RUN )
	{	if( reg.limit - u32AHI_TickTimerRead() >= reg.value )
		{
			reg.tstat &= (~TSTAT_TIM_RUN);
			// ���������� ������ ������ ���������� ������ ��� �����������
			registration_timeout();
			// ���� �� ������� ��� ����, ��� �� ��������� '������' ���� ������ ������������������
			// � ��������� ���� ������� ��� ���������������.
			// ���� �� ������� ����������, ���� ����� �� ����������������
			run_registration_timer( 15 );

		}
	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
static void run_registration_timer( ulong interval_sec )
{
	reg.value = interval_sec * SYS_FREQUENCY - SYS_FREQUENCY/20UL;
	reg.limit = u32AHI_TickTimerRead() + reg.value;
	reg.tstat |= TSTAT_TIM_RUN;
}


//---------------------------------------------------------------------------
//  ���������� �������� �������������. ������� �� �������� ������ � ������
// � �������� '���������'. ������� ����� ���� 1/8 ���������� �������� �����
// ���������� ���������
//---------------------------------------------------------------------------
static void registration_timeout(void)
{
int idx = bif.idx_handle;
int quality_best = 0;

if( chk_debug( DL_QUALITY ) )
{
xprintf( "\r--- %4X [%1X]\r", bif.rec[bif.idx_handle].addr, bif.idx_handle );
bif_list();
}

	bif.idx_best = HANDLE_INVALID;
	bif.quality = ( bif.rec[idx].cnt_bi - bif.rec[idx].cnt_bir )<< bif.rec[idx].cod_slots;
	// ��������� ����
	bif.quality += bif.quality/8;


	for( ; ; )
	{   // ������������� '�����', �� ���� ������� � �������� ��������������� ������ � �����
		if( (--idx) < 0 )
			idx = bif.idx_max;

		if( idx == bif.idx_handle )
		{	bif.rec[idx].cnt_bi  = 0;
			break;
		}

		if( bif.rec[idx].cnt_bi <= 0 )
		{   // �������� ������, �������� �� �������� ����������� �� ���� �������� ������� ������
			bif.rec[idx].addr = 0;
			bif.rec[idx].cnt_bir = 0;
		}
		else
		{  	int quality = (( bif.rec[idx].cnt_bi - bif.rec[idx].cnt_bir )<< bif.rec[idx].cod_slots );
	   		if( quality < 0 )
				quality = 0;
			if( bif.idx_best <= HANDLE_INVALID )
			{	// ���� ������ �������� � ������ ����
				if(  quality > bif.quality )
				{	bif.idx_best = idx;
					quality_best = quality;
				}
			}
			else
			{	// ���� ����� ����� ��������� '�������'
				if( quality > quality_best )
				{	bif.idx_best = idx;
					quality_best = quality;
				}
			}
		}
		bif.rec[idx].cnt_bi  = 0;
	}

	if( bif.idx_best <= HANDLE_INVALID )
		bif.idx_best = bif.idx_handle;

	bif.rec[bif.idx_best].state = BIF_STATE_REQ;
	bif.rec[bif.idx_best].cnt_bir = 0;

if( chk_debug( DL_QUALITY ) )
{
xprintf( "Top:%4X [%1X] L:%5u\r", bif.rec[bif.idx_best].addr, bif.idx_best, bif.quality>>4 );
//bif_list();
}

}

//---------------------------------------------------------------------------
//  ���������� ��� ��������� ������������� � �������� ��������� ������
//---------------------------------------------------------------------------
void confirm_callback( BYTE handle, bint result )
{

 	if( handle <= bif.idx_max )
	{ 	// X������� �� ���� �������� BIR
		if( result == MAC_ENUM_SUCCESS )
		{	// ������������� �������� - Store new Beacon
			if( ( bif.rec[handle].addr )&&(( handle == bif.idx_handle )||( handle == bif.idx_handle_data )) )
			{	// �� handle ���� �����.
				int idx = handle;

//#if( USE_DATA_UART )
//				if( bif.rec[idx].state == BIF_STATE_SEND_DATA )
//				{	// ����������� �������� ������ �������� UART
//					set_ud_state( UD_STATE_IDLE );
//				}
//#endif
				bif.rec[idx].state = BIF_STATE_WAIT;

				for( ; ; )
				{	// ������� �������� BI �������� ��� ���� �������
					bif.rec[idx].cnt_bi  = 0;
					if( --idx < 0 )
						idx = bif.idx_max;
					if( idx == handle )
						break;
				}

				// ������ ������������� ���������
//#if( USE_BI_EXTENDED )
				run_registration_timer( bif.rec[idx].reg_timeout );
//#else
//				run_registration_timer( ( bif.rec[idx].features & DF_LAMP_ROOM ) ? TIME_REREG_DEFAULT_LAMPROOM : cfg.dev_config.time_reg );
//#endif

if( chk_debug( DL_QUALITY ) )
{
xprintf( "Ack>%4X [%1X]\t\t\t\t%s\r", bif.rec[handle].addr, handle, get_time_string( unix_timer ) );
}

			}
			else
			{	xprintf( "xIR:%4X [%1X] Invalid Handle\r", bif.rec[handle].addr, handle );
				sw_set( STW_RESET_REQUEST );
			}
		}
		else
		{
if( chk_debug( DL_DUMMY ) )
	xprintf( "BIR:%4X [%1X] No ACK %2X\r", bif.rec[handle].addr, handle, result );
		}
	}
	else if( handle == HANDLE_IIR )
	{

if( chk_debug( DL_DUMMY ) )
	xprintf( "IIR:%4X ACK\r", bif.rec[handle].addr );


	}

}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void bif_list(void)
{
int idx = bif.idx_handle;  // ��������� ��������� ������

xprintf( "    %4X [%1X] L:%5u [%1X]\r", bif.rec[bif.idx_handle].addr, bif.idx_handle, bif.quality>>4, bif.rec[bif.idx_handle].state );
	for( ; ; )
	{   // ����� ������ ������� � ������ �������, ���� ��������� � ������ �������� ��������� ������������������
		if( (++idx) > bif.idx_max )
			idx = 0;

		if( bif.rec[idx].cnt_bi || bif.rec[idx].addr )
		{ 	// ����� � ���� ���������, ��� ������ ����������
			int quality = (( bif.rec[idx].cnt_bi - bif.rec[idx].cnt_bir )<< bif.rec[idx].cod_slots );
			if( quality < 0 )
				quality = 0;

xprintf( "    %4X [%1X] Q:%5u I:%3u R:%3u [%1X]\r",    bif.rec[idx].addr, idx,
												       quality>>4,
													   bif.rec[idx].cnt_bi, bif.rec[idx].cnt_bir, bif.rec[idx].state );
		}

	   	if( idx == bif.idx_handle )
			break;
	}
}

//---------------------------------------------------------------------------
//  BI Filter initialization
//---------------------------------------------------------------------------
void bif_init(void)
{
	bif.idx_handle 		= 0;
	bif.idx_best 		= HANDLE_INVALID;
	bif.idx_max     	= cfg.dev_config.list_bif - 1;
	bif.rec_cnt     	= 0;
	// ������ �� ���� ������������
	if( bif.idx_max > HANDLE_BIR_MAX )
		bif.idx_max = HANDLE_BIR_MAX;

	for( bint i=0; i < BIF_NNN_RECORDS; i++ )
	{	//bif.rec[i].time_end = bif.rec[i].time_new = 0UL;
		bif.rec[i].state   = BIF_STATE_WAIT;
		bif.rec[i].addr	   = 0;
		bif.rec[i].cod_slots	   = 4;  // ����������� ���������� ���������� = 16
		bif.rec[i].cnt_bi  = 0;
		bif.rec[i].cnt_bir = 0;
	}
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void rx_data_parser( rx_frame_s *f )
{

	dprintx_header( DL_RXD_FRAME );

	if( chk_debug( DL_RXD_FRAME ) )
	{  	printst( "   :" );	// Frame body print
		if( f->len )
			print_buff( f->buff, f->len );
		print_ch( '\r' );
	}

	switch( f->cmd )
	{


	//---------------------------------------------------------
	case CMD_BI:

		dprintf( DL_TXD_RXD_BI, "CMD>%2X:%4X BI    (%4X) D:%4Xr", f->dev_type, f->src.addr,
							f->dev_info_union );
		if( f->bi.cod_group  )
		{	// ���� � ������� ���� ������� �� ������ �������� �� ������������ ������� ������
			if( ( cfg.airid.id &( ~(0xFFFFFFFFUL << f->bi.cod_group ) ) ) != f->bi.cur_group )
			{	// ID ���� �� � ���� ������
				// ����� ���� �� ������ BI ������� ������� ��� ������ ��������
				inc_bi_counter( f );
				break;
			}
		}

		switch( chk_beacons_list( f ) )
		{
		case BIF_STATE_REQ:

			send_bir( bif.idx_handle, NULL, 0 );

			break;

		}
		break;


	//---------------------------------------------------------
	default:
		if( cfg.debug & ( DL_RXD_FRAME ) )
		{
		 	dprintx_header( DL_RXD_FRAME );

			printst( "   :" );  	// Frame body print
			if( f->len )
				print_buff( f->buff, f->len );
			outchar( '\r' );
		};

		if( ( f->dst.addr == cfg.airid.id )&&( f->src.addr != AIR_BROADCAST_ID ) )
		{	// ������ �����-�� ����������� ����� � ���� �������
			send_error( f->src.addr );
		}
	}
}




