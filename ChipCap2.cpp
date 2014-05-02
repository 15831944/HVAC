#include "ChipCap2.h"
#include "LCD.h"
#include <stdio.h>
#include <string.h>
#include "Arduino.h"
#include <Wire.h>

#define CCDZ 4

void ChipCapInit( )
{
  // Enable I2C (SDA=A4,SCL=A5)
  Wire.begin( ); 
  
  // ChipCap sensor power
  pinMode(13, OUTPUT);
  digitalWrite( 13, HIGH );
  delay( 500 );
  
  // ChipCapSetAddress( 0x28, 0x2A );
}

bool ChipCapSetAddress( int oldAddress, int newAddress )
{
  int i=0;
  uint8_t data[3];

  // Power cycle the ChipCap sensor to reset the command window
  digitalWrite( 13, LOW );
  delay( 500 );
  digitalWrite( 13, HIGH );
  
  // Give 1ms for the chip to wakeup
  delay( 1 );
  
  // Send Start Command Mode
  Wire.beginTransmission( oldAddress );
  Wire.write( 0xA0 );
  Wire.write( 0x00 );
  Wire.write( 0x00 );
  Wire.endTransmission( );
  
  // Give 1ms (Min req. 100uS)
  delay( 1 );
  
  // Read the status of the Start CM command
  Wire.requestFrom( oldAddress, 1, true );
  if( Wire.available( ))
  {
    data[0] = Wire.read( );    
    if( data[0] != 0x81 ) ShowError( 1, data[0], 0,0 );      
  }
  else ShowError( 2, 0, 0, 0 );
    
  // Read the Customer Configuration word
  Wire.beginTransmission( oldAddress );
  Wire.write( 0x1C );
  Wire.write( 0x00 );
  Wire.write( 0x00 );
  Wire.endTransmission( );
  
   // Give 1ms (Min req. 100uS)
  delay( 1 );
  
  Wire.requestFrom( oldAddress, 3, true );
  while( Wire.available( ) && i<3 )
  {
    data[i++] = Wire.read( );
  }
  
  // Check that the address is correct
  if(( data[0] != 0x81 ) || (( data[2] & 0x7F ) != oldAddress )) ShowError( 3, data[0], data[1], data[2] );
  
  // Write the new address
  Wire.beginTransmission( oldAddress );
  Wire.write( 0x5C );
  Wire.write( data[1] );
  Wire.write( newAddress | ( data[2] & 0x80 ));
  Wire.endTransmission( );
  
  // Give 15ms (Min req. 12ms)
  delay( 15 );
  
  // Check the status of the Write command
  Wire.requestFrom( oldAddress, 1, true );  
  if( Wire.available( ))
  {
    data[0] = Wire.read( );    
    if( data[0] != 0x81 ) ShowError( 4, data[0], 0,0 );      
  }
  else ShowError( 5, 0, 0, 0 );

  // Exit command mode
  Wire.beginTransmission( oldAddress );
  Wire.write( 0x80 );
  Wire.write( 0x00 );
  Wire.write( 0x00 );
  Wire.endTransmission( );
  
  delay( 1 );
  
  return true;
}

void ChipCapRead( int address, int& temp, int& RH )
{
  int i=0;
  uint8_t data[CCDZ];
  
  Wire.requestFrom( address, CCDZ, true );
  while( Wire.available( ) && i<CCDZ )
  {
    data[i++] = Wire.read( );
  }
  if( i==CCDZ )
  {
    RH = ((((uint32_t)data[0] << 8) + (uint32_t)data[1]) * 100 ) >> 14;
//    temp = (((((uint32_t)data[2] << 6) + ((uint32_t)data[3] >> 2)) * 165 ) >> 14 ) - 40;
    
    temp = (((((uint32_t)data[2] << 6) + ((uint32_t)data[3] >> 2)) * 825  ) >> 13 ) - 400;   
  }
}
