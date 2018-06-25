

#ifndef __SYSTEM_ISIB_JNX_H
#define __SYSTEM_ISIB_JNX_H


#ifdef __GNUC__
 #if( __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ )
  #define __LITTLE_ENDIAN__	 TRUE
  #define __BIG_ENDIAN__	 FALSE
 #else
  #define __BIG_ENDIAN__	 TRUE
  #define __LITTLE_ENDIAN__	 FALSE
 #endif
#endif

#define SYS_FREQUENCY      (16000000UL)

typedef unsigned int      		uint;
typedef int       				bint;
typedef int 					bshort;
typedef unsigned long       	ulong;
typedef unsigned short      	ushort;
typedef unsigned long			DWORD;
typedef unsigned short			WORD;
typedef unsigned char  			uchar;
typedef unsigned char  			BYTE;


#define ARGSUSED(x)	(void)(x)

#define BIT(n)           ( 1 <<   (n) )
#define SETBIT( p, n )   ( p|= BIT(n) )
#define CLRBIT( p, n )   ( p&=~BIT(n) )
#define XORBIT( p, n )   ( p=p^BIT(n) )
#define TSTBIT( p, n )   ( p&  BIT(n) )


#define BIT0		BIT(0)
#define BIT1		BIT(1)
#define BIT2		BIT(2)
#define BIT3		BIT(3)
#define BIT4		BIT(4)
#define BIT5		BIT(5)
#define BIT6		BIT(6)
#define BIT7		BIT(7)
#define BIT8		BIT(8)
#define BIT9		BIT(9)
#define BIT10   	BIT(10)
#define BIT11		BIT(11)
#define BIT12		BIT(12)
#define BIT13		BIT(13)
#define BIT14   	BIT(14)
#define BIT15		BIT(15)
#define BIT16		BIT(16)
#define BIT17		BIT(17)
#define BIT18   	BIT(18)
#define BIT19		BIT(19)
#define BIT20		BIT(20)
#define BIT21		BIT(21)
#define BIT22   	BIT(22)
#define BIT23		BIT(23)
#define BIT24		BIT(24)
#define BIT25		BIT(25)
#define BIT26   	BIT(26)
#define BIT27		BIT(27)
#define BIT28		BIT(28)
#define BIT29		BIT(29)
#define BIT30   	BIT(30)
#define BIT31		0x80000000


extern 		ulong 	unix_timer;
extern volatile ulong  	sys_status_word;
extern struct sys_state_s sys;


#define htons(x)  (x)
#define ntohs(x)  (x)
#define htonl(x)  (x)
#define ntohl(x)  (x)



/*
// Enable/disable external and tick timer interrupts
#define enable_interrupts();                                           	    	\
    {                                                                      	    \
        register uint32 rulCtrlReg;                             				\
        asm volatile ("b.mfspr %0, r0, 17;" :"=r"(rulCtrlReg) : );            	\
        rulCtrlReg |= 0x6;                                                   	\
        asm volatile ("b.mtspr r0, %0, 17;" : :"r"(rulCtrlReg));           	    \
    }

#define disable_interrupts();                                               	\
    {                                                                           \
        register uint32 rulCtrlReg;                          	    			\
        asm volatile ("b.mfspr %0, r0, 17;" :"=r"(rulCtrlReg) : );              \
        rulCtrlReg &= 0xFFFFFFF9;                                               \
        asm volatile ("b.mtspr r0, %0, 17;" : :"r"(rulCtrlReg));                \
    }

#define enable()        enable_interrupts()
#define disable()       disable_interrupts()
*/


#define enable()        MICRO_ENABLE_INTERRUPTS()
#define disable()       MICRO_DISABLE_INTERRUPTS()

#define disable_nvr()	disable()
#define enable_nvr()	enable()


/*
#define disable_uart1()	    vAHI_InterruptSetPriority( MICRO_ISR_MASK_UART1,  0 );
#define enable_uart1()	    vAHI_InterruptSetPriority( MICRO_ISR_MASK_UART1,  UART1_INT_PRIORITY );
*/


#define ENTER_CRITICAL()  { ulong _saved_i_status_;		   	   				\
					   		MICRO_DISABLE_AND_SAVE_INTERRUPTS( _saved_i_status_ )


#define LEAVE_CRITICAL()  	MICRO_RESTORE_INTERRUPTS( _saved_i_status_ ); 	 \
						  }


//---------------------------------------------------------------------------
#define	sw_chk(x)  		( sys_status_word & (x) )
#define	sw_get(x)  		( sys_status_word      )
#define	sw_set_isr(x)  	sys_status_word |= (x)
#define	sw_clr_isr(x) 	sys_status_word &= (~(x))

#define	sw_set(x)	{  	unsigned long i_status_;		   	 			\
						MICRO_DISABLE_AND_SAVE_INTERRUPTS( i_status_ ); \
						sys_status_word |= (x)							\
						MICRO_RESTORE_INTERRUPTS( i_status_ );  }

#define	sw_clr(x)	{  	unsigned long i_status_;						\
				   		MICRO_DISABLE_AND_SAVE_INTERRUPTS( i_status_ ); \
				   		sys_status_word &= (~(x))						\
				   		MICRO_RESTORE_INTERRUPTS( i_status_ );  }


//---------------------------------------------------------------------------
#define	bs_chk(x)  		( sys.main.state & (x) )
#define	bs_get(x)  		( sys.main.state       )
#define	bs_set_isr(x)  	sys.main.state |= (x)
#define	bs_clr_isr(x) 	sys.main.state &= (~(x))
#define	bs_set(x)	{  	unsigned long i_status_;		   	 			\
					   		MICRO_DISABLE_AND_SAVE_INTERRUPTS( i_status_ ); \
					   		sys.main.state |= (x)							\
							MICRO_RESTORE_INTERRUPTS( i_status_ );  }
#define	bs_clr(x)	{  	unsigned long i_status_;						\
					   		MICRO_DISABLE_AND_SAVE_INTERRUPTS( i_status_ ); \
					   		sys.main.state &= (~(x))						\
							MICRO_RESTORE_INTERRUPTS( i_status_ );  }


//---------------------------------------------------------------------------
bint init_main_timer(void);
bint init_unix_timer(void);

//---------------------------------------------------------------------------
typedef struct {
	ulong limit;
	ulong value;
	ulong tstat;
} sys_timer_s;

//---------------------------------------------------------------------------
void set_timer( sys_timer_s *tim, ulong value );
void set_timer_go( sys_timer_s *tim, ulong value );
void stp_timer( sys_timer_s *tim );
void rst_timer( sys_timer_s *tim );
bint chk_timer( sys_timer_s *tim );
bint chk_timer_stop( sys_timer_s *tim );
bint chk_timer_restart( sys_timer_s *tim );
void chg_timer( sys_timer_s *tim, ulong value );
void res_timer( sys_timer_s *tim );


#define set_timer_ms( t, v )	set_timer( (t), ((v)*(    (SYS_FREQUENCY)/1000UL)) )
#define set_timer_ts( t, v )	    set_timer( (t), ((v)*((6)*(SYS_FREQUENCY)/1000UL)) )
#define set_timer_go_ts( t, v )	set_timer_go( (t), ((v)*((6)*(SYS_FREQUENCY)/1000UL)) )


extern sys_timer_s slot_timer;
extern sys_timer_s ledx_timer;
extern sys_timer_s regs_timer;
extern sys_timer_s ccir_timer;


//---------------------------------------------------------------------------
// Функции работы со временем
extern volatile ulong main_timer_cnt;
extern volatile ulong unix_timer_cnt;

struct tm *tmtime( ulong utime );
extern struct tm timex;
char *get_time_string( ulong utime );
void reset( bint flags );
#define set_unix_time( x )  		unix_timer_cnt = (x)
#define	get_unix_time()    			unix_timer_cnt
#define	get_unix_time_network()    	unix_timer_cnt

void dummy_delay( bint x );
void flash_invalidate(void);
bint check_protection(void);
void blocking_set( bint block );

#endif

