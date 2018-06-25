

#ifndef __CRC32_AKVA_H
#define __CRC32_AKVA_H


ulong crc32( ulong crc, const void *buf, int size );
void crc32_add( ulong *crc_ext, BYTE data );


extern const BYTE crc8_tab[256];

BYTE crc8( BYTE *buff, bint len );

#define crc8_add( crc, data )   crc8_tab[(crc) ^ (data)]

#define	get_ttx_signature( x, c ) crc8_tab[ (BYTE)((x)+((x)>>2)) ^ (c) ]


#endif
