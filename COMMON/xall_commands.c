
#if( USE_BLOCKING_TRAP )

//-------------------------------------
case 'lim ':
	switch( cmd->argc  )
	{
	case 2:
		switch( cmd->argn[ARGSS] )
		{
		case 1:
			cfg.dist.lim1.tof  = bcd2bin(cmd->argn[1]);
			cfg.dist.lim1.rssi = bcd2bin(cmd->argn[2]);
			break;

		case 2:
			cfg.dist.lim2.tof  = bcd2bin(cmd->argn[1]);
			cfg.dist.lim2.rssi = bcd2bin(cmd->argn[2]);
			break;

		case 3:
			cfg.dist.lim3.tof  = bcd2bin(cmd->argn[1]);
			cfg.dist.lim3.rssi = bcd2bin(cmd->argn[2]);
			break;

		case 4:
			cfg.dist.lim4.tof  = bcd2bin(cmd->argn[1]);
			cfg.dist.lim4.rssi = bcd2bin(cmd->argn[2]);
			break;

		}
		break;

	default:
		goto cmd_error;

	}
	break;

//-------------------------------------
case 'tout':

	switch( cmd->argc  )
	{
	case 1:
		switch( cmd->argn[ARGSS] )
		{
		case 1:
			cfg.tout.t1  = bcd2bin(cmd->argn[1]);
			break;

		case 2:
			cfg.tout.t2  = bcd2bin(cmd->argn[1]);
			break;

		case 3:
			cfg.tout.t3  = bcd2bin(cmd->argn[1]);
			break;

		case 4:
			cfg.tout.t4  = bcd2bin(cmd->argn[1]);
			break;
		}
		break;

	default:
		goto cmd_error;

	}
	break;

#endif


#if( USE_TOF > 1 )
//-------------------------------------
case 'mode':

	switch( cmd->argc  )
	{
	case 1:
		switch( cmd->argn[ARGSS] )
		{
		case 1:
			cfg.mode.m1  = cmd->argn[1];
			break;

		case 2:
			cfg.mode.m2  = cmd->argn[1];
			break;

		case 3:
			cfg.mode.tof_nnn  = bcd2bin(cmd->argn[1]);

		case 4:
			cfg.mode.tof_mod  = cmd->argn[1];
			break;
		}
		break;

	default:
		goto cmd_error;

	}
	break;
#endif

//-------------------------------------
case 'debu':
	switch( cmd->argc )
	{
	case 1:
		cfg.debug = cmd->argn[1];
	case 0:
		xprintf( "SDL:%8X\r", cfg.debug );
		break;
	case 2:
		if( cmd->argn[2] )
			cfg.debug |=  ( 1 << bcd2bin(cmd->argn[1]) );
		else
			cfg.debug &= ~( 1 << bcd2bin(cmd->argn[1]) );
		xprintf( "SDL:%8X\r", cfg.debug );
		break;

	default:
		goto cmd_error;
	}
	break;

//-------------------------------------
case 'stw ':
	switch( cmd->argc )
	{
	case 0:
		xprintf( "SDL:%8X\r", sw_get() );
		break;
	case 2:
		if( cmd->argn[2] )
		{
			sw_set( 1 << bcd2bin(cmd->argn[1] ) );
		}
		else
		{
			sw_clr( 1 << bcd2bin(cmd->argn[1] ) );
		}
		xprintf( "SDL:%8X\r", sw_get() );
		break;

	default:
		goto cmd_error;
	}
	break;

#if( DEV_ISIB2 )
#else
//-------------------------------------
case 'ps ':
	println( "Ok" );
	break;

//-------------------------------------
case 'rese':
	if( cmd->argc == 1 )
	{
		if( cmd->argn[1] == 0x4321 )
		{   println( "Full Reprogramm..." );
			flash_invalidate();
			print_all();
			reset( 0 );
		}
		else if( cmd->argn[1] == 0xEEEE )
		{   println( "EEPROM to Default..." );
			cfg.signature = 0;
			if( eeprom_cfg_store() == 0 )
				println( "Ok" );
			print_all();
			reset( 0 );
		}
		else if( cmd->argn[1] == 0xEEEF )
		{   println( "EEPROM & Ident to Default..." );
			cfg.signature = 0;
			cfg.airid_long = 0;
			if( eeprom_cfg_store() == 0 )
				println( "Ok" );
			print_all();
			reset( 0 );
		}
#if( DEV_ISBRF || DEV_TAGXX || DEV_IVT24 )
#else
		else if( cmd->argn[1] == 0xFFFF )
		{   println( "FRAM Check & Erase..." );
			print_all();

			fram_simple_test();
			init_fram();
			xprintf( "Done\r" );
		}
		else
#endif
			reset( cmd->argn[1] );
	}
	else
		reset( 0 );

	break;
#endif

#if( DEV_TAGXX || DEV_IVT24 )
#else

//-------------------------------------
case 'sinf':
		if( cmd->argc != 2 )
	   		goto cmd_error;

		nvr_store_info( cmd->argn[1], cmd->argn[2] );

		break;

//-------------------------------------
case 'sdat':
		if( cmd->argc != 2 )
	   		goto cmd_error;

static BYTE dummy_data[] = { 0xAA, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
							 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xAA	 };

//		++dummy_data[0];
		nvr_store_data( (f_tag_id_u)1234UL, &dummy_data[0], bcd2bin(cmd->argn[1] ), cmd->argn[2] );

		break;

//-------------------------------------
case 'seve':
		if( cmd->argc != 2 )
	   		goto cmd_error;

static f_tag_id_u tag;

		tag.tid_type = 0x24;
		tag.tid_addr = 0x1;
		tag.tid_ext_ident = 0;

		nvr_store_event( tag, cmd->argn[1], cmd->argn[2] );

		break;

//-------------------------------------
case 'ssss':

		tag.tid_type = 1;
		tag.tid_addr = 1;
		tag.tid_ext_ident = 0;

		nvr_store_event( tag, 0, 0 );

		break;

//-------------------------------------
case 'stop':
	bs_clr( BSF_MODE_WORK );
	bs_set( BSF_MODE_PASSIVE );

	break;



/*
#if( ( DEV_ISBRF == 3 )||( DEV_ISBRF == 5 ) )
//-------------------------------------
case 'cc  ':

	if( cmd->argc != 1 )
		goto cmd_error;

	send_cc_zone( 0x21, 500, cmd->argn[1] );

	break;
#endif
*/


//-------------------------------------
case 'tii ':

	if( cmd->argc < 1 )
		goto cmd_error;

	if( cmd->argc <= 1 )
		cmd->argn[2] = 0;

	tof_ii_start( bcd2bin(cmd->argn[1]), cmd->argn[2] );

	break;

//-------------------------------------
case 'dc  ':

	if( cmd->argc < 2 )
		goto cmd_error;

	if( cmd->argc == 2 )
		cmd->argn[3] = 0xFFFFFFFFUL;

	send_dc( bcd2bin(cmd->argn[1] ), cmd->argn[2], cmd->argn[3]  );

	break;

#endif


//-------------------------------------
#if( USE_TOF > 1 )

case 'ttl ':


	if( cmd->argc == 1 )
	{
		tof.loops = bcd2bin( cmd->argn[1] ) - 1;
	}
	else
		tof.loops = 0;

	sw_set( STW_TOF_TESTER );



	break;

//-------------------------------------
case 'tof ':

	cfg.debug |=  BIT6;

	switch( cmd->argc  )
	{
  	case 0:

		tof_start_cmd( tid, tof_mode );
//		tof_start( tid );

		break;

	case 1:

		tid = bcd2bin( cmd->argn[1] );
	 	tof_start_cmd( tid, tof_mode );
//		tof_start( tid );
		break;


	case 2:
		tid = bcd2bin( cmd->argn[1] );
		tof_mode = bcd2bin( cmd->argn[2] );
		tof_start_cmd( tid, tof_mode );
		break;

	default:
		goto cmd_error;

	}
	break;
#endif

#if( DEV_TAGXX || DEV_IVT24 )
//-------------------------------------
case 's   ':
		if( cmd->argc  != 1 )
			goto cmd_error;
		if( cfg.airid.id  == TAG_AIR_DEFAULT_ID  )
		{	cfg_set_air_id( bcd2bin( cmd->argn[1] ) );
			break;
		}
		goto cmd_error;
#endif

//-------------------------------------
case 'cfg ':
case 'set ':

	switch( cmd->argc  )
	{
	case 0:

		cfg_dump();

		break;

	case 1:
		switch( cmd->argn[ARGSS] )
		{
		case 'W':
			if( eeprom_cfg_store() == 0 )
				println( "Ok" );
			else
				println( "Failed" );
			break;

#if( DEV_TAGXX || DEV_IVT24 )

#if( DEV_IVT24 )
		case 'S':
			cfg.airsc.timeslots_cod = bcd2bin( cmd->argn[1] )/100;
			if( cfg.airsc.timeslots_cod < 10 )
				cfg.airsc.timeslots_cod = 10;
			xprintf( "Scan:%5ums\r", cfg.airsc.timeslots_cod*100 );
			break;
#endif


#if( DEV_TAGXX )
		case 'L':
	  	   	if( cfg_set_config_full( (tag_config_u)cmd->argn[1] ) )
				goto cmd_error;
			break;
#endif

		case 'T':
			cmd->argn[1] = bcd2bin(cmd->argn[1]);
			if( ( cmd->argn[1] < 2 )||( cmd->argn[1] >= 60*30 ) )
				goto cmd_error;
			cfg.dev_config.time_reg = cmd->argn[1];
			break;
#else
		case 0xE:
			cfg.airsc.cod_group = bcd2bin(cmd->argn[1]);
			break;
#endif

		case 'I':
			cfg_set_air_id( bcd2bin( cmd->argn[1] ) );
			break;

#if( DEV_ISIB2 || DEV_ISIB3 )
		case 'S':
			cfg_set_serialn( bcd2bin( cmd->argn[1] ) );
			break;

		case 0xB:
			cfg_set_baud( bcd2bin( cmd->argn[1] ) );
			break;
#endif

		case 0xC:
			cfg.airch.ch = (BYTE)bcd2bin( cmd->argn[1] );
			break;

		case 0x0:
			cfg.tof_offset  = bcd2bin( cmd->argn[1] );
			break;

		case 0xF:
			cfg.dev_config.features = cmd->argn[1];
			break;

		case 0xD:
			if( cmd->argn[1] < 0x24 )
				goto cmd_error;

			cfg.airch.dev_type = cmd->argn[1];
			break;

		case 'P':
			cfg.airch.out_power = cmd->argn[1];
			break;


		default:
			goto cmd_error;

		}
		break;

	case 2:
		switch( cmd->argn[ARGSS] )
		{
#if( DEV_TAGXX )
		case 'L':
		   	if( cfg_set_config( cmd->argn[1], cmd->argn[2] ) )
				goto cmd_error;

			break;
#else
		case 'K':
			if( cmd->argn[1] < IOCH_NN )
			 	cfg.iocfg[cmd->argn[1]].io_cfg = cmd->argn[2] & 0xF0;
			else
				goto cmd_error;
#endif

			break;

		default:
			goto cmd_error;

		}
		break;

#if( DEV_ISIB2 || DEV_ISIB3 )
	case 3:
		switch( cmd->argn[ARGSS] )
		{

		case 0xA:

			if( bcd2bin( cmd->argn[1] ) > 63 )
				goto cmd_error;

			if( bcd2bin( cmd->argn[2] ) > 15 )
				goto cmd_error;

			if( bcd2bin( cmd->argn[3] ) > 15 )
				goto cmd_error;


			cfg_set_beacon_address_store( bcd2bin( cmd->argn[1]), bcd2bin( cmd->argn[2]), bcd2bin( cmd->argn[3] ) );

		   	break;

		default:
			goto cmd_error;

		}
		break;
#endif


	default:
		goto cmd_error;

	}
	break;

//-------------------------------------
case 'time':
	xprintf( "%s\r", get_time_string( unix_timer ) );
	break;


//-------------------------------------
case 'ps  ':
	println( "Ok" );
	break;

//---------------------------------------------------------------------------

//--- Dummy arguments print -----------
case 'aa  ':
	xprintf( "\nargc=%1u ls='%s'\r", cmd->argc, cmd->command_line_orig );
	xprintf( "argn[s]=%8X\r", cmd->argn[0] );
	for( i=1; i<= cmd->argc; i++ )
		xprintf( "argn[%1u]=%8X\r", i, cmd->argn[i] );
	break;


//--- mode ------------------------------------------------------------------
case 'dmod':
	if( cmd->argc )
	{	switch( cmd->argn[1] )
		{
	case 1:
	case 2:
	case 3:
	case 4:
				mode_wb = cmd->argn[1];
				break;
	case 'I':
				if( cmd->argc == 2 )
					mode_io = (mode_io & 0x0080)|(cmd->argn[2] & 0x0007);
				else
					mode_io ^= 0x80;
				break;
	default:
				goto cmd_error;

		}
	}
	dumpstatus();

	break;

//--- set bit ---------------------------
case 'sb  ':
	if( cmd->argc != 3 )
		goto cmd_error;

	if( cmd->argn[ARGSS] != 0xFFFFFFFF )
		base_io = set_base_io( cmd->argn[ARGSS] );
	{
	ulong vlong, mbit;
	ulong *ptr = (ulong *)(base_io+cmd->argn[1]);
	mbit = bcd2bin( cmd->argn[2] );
	mbit = 1<<mbit;
	vlong = *ptr;

	switch( cmd->argn[3] )
	{
case 0:           	        // Set to 0
		vlong &= ~mbit;
		break;
case 1:                     // Set to 1
		vlong |= mbit;
		break;
case 3:                		// Reverse BIT
		vlong ^= mbit;
		break;
default:
		goto cmd_error;
	}
	*ptr = vlong;
	}
	break;

//--- d ump ---------------------------
case 'd   ':
	i = 16;
	switch( cmd->argc )
	{
	case 2:
			i = cmd->argn[2]-cmd->argn[1]+1;
			if( i < 0  )
				i = cmd->argn[2];
			if( (i<=0)||(i>0xB0) )
				goto cmd_error;
	case 1:
			offs_io = cmd->argn[1];
	case 0:
			break;
	default:
			goto cmd_error;
	}

	if( cmd->argn[ARGSS] != 0xFFFFFFFF )
		base_io = set_base_io( cmd->argn[ARGSS] );
	if( base_io >= 0xE0000000 )
		mode_wb	= 4;
	if( mode_wb == 3 )
		opsize = 1;
	else
		opsize = mode_wb;

	dumpmem( base_io, offs_io, i, mode_wb );
	offs_io += opsize;

	break;

