
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
// Отдельный счетчик запросов BI.
// Вызывается, если таг получил запрос, но таг находится не в запрашивамой
// группе
//---------------------------------------------------------------------------
static void inc_bi_counter( rx_frame_s *f )
{
// Последний подтвержденный индекс - bif.idx_handle. C него начинаем поиск в списке
int idx = bif.idx_handle;
	do{
		if( bif.rec[idx].addr )
		{	// Есть активная запись.
			if( bif.rec[idx].addr == f->src.addr )
			{   // Бикон найден в таблице активных.
				// Безусловно увеличиваем счетчик BI запросов данного бикона
				bif.rec[idx].cnt_bi++;
			}
		}
		// Поиск назад от текущего
		if( (--idx) < 0 )
			idx = bif.idx_max;
	}
	while( idx != bif.idx_handle );
}

//---------------------------------------------------------------------------
// Superfluous BI requests filter.
// Вызвается при получении BI запроса. Считает количество запрсов и ответов
// в период между регистрациями для оценки качества связи с биконами.
// Занимается выделением хандлера для запроса по которому будут фиксироваться ACK.
// Здесь-же фильтруется избыточное количество биконов в зоне видимости тага
//---------------------------------------------------------------------------
static bint chk_beacons_list( rx_frame_s *f )
{
// Последний использованый индекс - bif.idx_handle. C него начинаем поиск в списке
int idx = bif.idx_handle;
bif.rec_cnt  = 0;
int idx_free = HANDLE_INVALID;

	do{
		if( bif.rec[idx].addr )
		{	// Есть активная запись. Увеличиваем счетчик активных записей.
			// Если в процессе поиска текущий бикон не будет найден с списке,
			// то счетчик записей будет показывать реальное количество записей
			// в таблице и может быть использован для анализа
			bif.rec_cnt++;

			if( bif.rec[idx].addr == f->src.addr )
			{   // Бикон найден в таблице активных.
				// Безусловно увеличиваем счетчик BI запросов на количество таймслотов в которых передает
				// данный бикон
				bif.rec[idx].cnt_bi++;

				if( bif.rec[idx].cnt_bir > MAX_BIR_RETRY )
				{   // Что то фатальное на ответы тага долго нет подтверждений.
					// Прекращаем передачу, но из списка бикон не выкидываем
xprintf( "DIS:%4X [%1X] [%1X]\r", bif.rec[idx].addr, idx, bif.rec[idx].state );
					bif.rec[idx].state = BIF_STATE_WAIT;
					bif.rec[idx].cnt_bir >>= 1;		// Уменьшаем счетчик, но оставляем "штраф"
					return( bif.rec[idx].state );
				}

				switch( bif.rec[idx].state )
				{
					//-------------------------------------------------------
				case BIF_STATE_REQ:
					// Назначаем nandle для запроса
					bif.idx_handle = idx;
					// Увеличиваем счетчик попыток BIR ответов
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
		{	// Запоминаем на всякий случай последнее найденное в списке свободное место
			// для записи нового бикона, когда он появится
			idx_free = idx;
		}
		// Поиск назад от текущего
		if( (--idx) < 0 )
			idx = bif.idx_max;
	}
	while( idx != bif.idx_handle );

	//-----------------------------------------------------------------------
	// Не найден в списке - появился новый бикон
	if( ( bif.rec_cnt <= bif.idx_max )&&( idx_free > HANDLE_INVALID ) )
	{	// Есть свободные места в списке - добавляем.
		// Но ответ не посылаем. Пошлем только при получении второго запроса.
		// Там так-же можно и нужно смотреть посылать ответ или нет

		idx = idx_free;


		bif.rec[idx].addr      = f->src.addr;			// Запоминаем адрес нового бикона.
		bif.rec[idx].cod_slots = f->bi.cod_slots;   	// Запоминаем период его повторения в таймслотах
 		bif.rec[idx].features  = f->dev_features;

//#if( USE_BI_EXTENDED )
		// В BI запросе дополнительный байт
		if( ( f->len == sizeof(bi_short_s) )||( f->bi.reg_timeout == 0 ) )
		{	// Время перегистрации не задано
			// Задаем период перерегистрации
			bif.rec[idx].reg_timeout = ( ( bif.rec[idx].features & DF_LAMP_ROOM ) ? TIME_REREG_DEFAULT_LAMPROOM : cfg.dev_config.time_reg );
		}
		else
			bif.rec[idx].reg_timeout = f->bi.reg_timeout * 5;   // Запоминаем период перерегистрации
//#endif

		bif.rec[idx].state = BIF_STATE_REQ;
		// Безусловно устанавливаем счетчики BI запросов полученных от этого бикона и счетчик BIR
		bif.rec[idx].cnt_bi  = 1;
		bif.rec[idx].cnt_bir = 0;

		return( BIF_STATE_WAIT );
	}
	else
	{	// Места в списке нет - игнорировать

		return( BIF_STATE_WAIT );
	}
}


//---------------------------------------------------------------------------
sys_timer_s reg;
#define TSTAT_TIM_RUN	(BIT0)
// Обработчик таймера перегистрации. В результате работы назначается новый
// лучший бикон из списка видимых и запускается процесс перегистрации
//---------------------------------------------------------------------------
void chk_registration(void)
{
	if( reg.tstat & TSTAT_TIM_RUN )
	{	if( reg.limit - u32AHI_TickTimerRead() >= reg.value )
		{
			reg.tstat &= (~TSTAT_TIM_RUN);
			// Собственно движек выбора следующего бикона для регистрации
			registration_timeout();
			// Фора по времени для того, что бы выбранный 'лучший' смог успеть зарегистрироваться
			// и выставить свой таймаут для перерегистрации.
			// Этот же таймаут отработает, если никто не зарегистрируется
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
//  Обработчик процесса перегистрации. Смотрит на активные биконы в списке
// и выбирает 'наилучший'. Текущий имеет фору 1/8 показателя качества перед
// остальными активными
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
	// Добавляем фору
	bif.quality += bif.quality/8;


	for( ; ; )
	{   // Просматриваем 'назад', то есть начиная с предущей регистрационной записи и ранее
		if( (--idx) < 0 )
			idx = bif.idx_max;

		if( idx == bif.idx_handle )
		{	bif.rec[idx].cnt_bi  = 0;
			break;
		}

		if( bif.rec[idx].cnt_bi <= 0 )
		{   // Выкинули запись, посколку за интервал регистрации не было получено запрсов бикона
			bif.rec[idx].addr = 0;
			bif.rec[idx].cnt_bir = 0;
		}
		else
		{  	int quality = (( bif.rec[idx].cnt_bi - bif.rec[idx].cnt_bir )<< bif.rec[idx].cod_slots );
	   		if( quality < 0 )
				quality = 0;
			if( bif.idx_best <= HANDLE_INVALID )
			{	// Ищем замену текущему с учетом форы
				if(  quality > bif.quality )
				{	bif.idx_best = idx;
					quality_best = quality;
				}
			}
			else
			{	// Ищем любой лучше найденого 'лучшего'
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
//  Вызывается при получении подтверждения о доставке сообщения бикону
//---------------------------------------------------------------------------
void confirm_callback( BYTE handle, bint result )
{

 	if( handle <= bif.idx_max )
	{ 	// Xандлеры из пула передачи BIR
		if( result == MAC_ENUM_SUCCESS )
		{	// Подтверждение получено - Store new Beacon
			if( ( bif.rec[handle].addr )&&(( handle == bif.idx_handle )||( handle == bif.idx_handle_data )) )
			{	// За handle есть бикон.
				int idx = handle;

//#if( USE_DATA_UART )
//				if( bif.rec[idx].state == BIF_STATE_SEND_DATA )
//				{	// Подтвердить передачу данных автомату UART
//					set_ud_state( UD_STATE_IDLE );
//				}
//#endif
				bif.rec[idx].state = BIF_STATE_WAIT;

				for( ; ; )
				{	// Счетчик запросов BI сбросить для всех записей
					bif.rec[idx].cnt_bi  = 0;
					if( --idx < 0 )
						idx = bif.idx_max;
					if( idx == handle )
						break;
				}

				// Таймер перегистрации запустить
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
int idx = bif.idx_handle;  // Последний записаный индекс

xprintf( "    %4X [%1X] L:%5u [%1X]\r", bif.rec[bif.idx_handle].addr, bif.idx_handle, bif.quality>>4, bif.rec[bif.idx_handle].state );
	for( ; ; )
	{   // Поиск вперед начиная с самого старого, дабы последним в списке оказался последний зарегистрированный
		if( (++idx) > bif.idx_max )
			idx = 0;

		if( bif.rec[idx].cnt_bi || bif.rec[idx].addr )
		{ 	// Бикон в зоне видимости, или только существует
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
	// Защита от сбоя конфигурации
	if( bif.idx_max > HANDLE_BIR_MAX )
		bif.idx_max = HANDLE_BIR_MAX;

	for( bint i=0; i < BIF_NNN_RECORDS; i++ )
	{	//bif.rec[i].time_end = bif.rec[i].time_new = 0UL;
		bif.rec[i].state   = BIF_STATE_WAIT;
		bif.rec[i].addr	   = 0;
		bif.rec[i].cod_slots	   = 4;  // Минимальное количество таймслотов = 16
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
		{	// Если в запросе есть деление на группы поверяем на соответствие текущей группы
			if( ( cfg.airid.id &( ~(0xFFFFFFFFUL << f->bi.cod_group ) ) ) != f->bi.cur_group )
			{	// ID тага не в этой группе
				// Здесь надо по любому BI запросы считать для оченки качества
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
		{	// Пришел какой-то неизвестный фрейм с моим адресом
			send_error( f->src.addr );
		}
	}
}




