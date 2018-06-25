

#ifndef __TOF_COMMS_H
#define __TOF__COMMS_H


#include <AppApiTof.h>

#define TOF_MAX_REQUESTS     (40)



#define TOF_OFFSET_BASE		 	(2000)
#define TOF_OFFSET_BASE_IVT24 	(1600)


void tof_init(void);
//bint tof_start_cmd( ushort addr, bint n, bool_t direction );
bint tof_start_cmd( ushort addr, BYTE tof_mode );
//bint tof_start_trap(void);
//bint tof_start_trap( bint phase );
bint tof_start( ushort addr, bint phase );
//bint tof_start( ushort addr );
void tof_ii_start( ushort addr, BYTE flag );
//bint tof_request(void);
bint tof_request(void);
//bint process_tof( int show );
void tof_uni( ushort addr );
bint process_tof_double( bint show );
void tof_tester(void);




//---------------------------------------------------------------------------
typedef struct {
	MAC_Addr_s 		addr;		// Структура для вызова координатора TOF
	bint			req_n;
//	bint			tof_err_cnt;
	bool_t    		direction;
	bool_t    		phase;
//	union {
//	tof_result_s;
//	ulong			distance_full;
//	};
//#if( USE_BLOCKING_TRAP && (USE_ADD_TRAPLIST == 0) )
//	bint 			idx;	// Индекс в таблице фильтра
//#endif
	tsAppApiTof_Data info[TOF_MAX_REQUESTS*2];

	signed long 	summ_tof;
	ulong 			summ_rss;
	bint 			rerr_cnt;
	ulong 			loops;

} tof_data_s;


extern tof_data_s	tof;



#endif

