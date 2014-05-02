#ifndef ChipCap2_h
#define ChipCap2_h

#include <inttypes.h>

void ChipCapInit( );
void ChipCapRead( int address, int& temp, int& RH );
bool ChipCapSetAddress( int oldAddress, int newAddress );

#endif
