
#include <Wire.h>
#include "LCD.h"
#include "ChipCap2.h"

typedef enum {
  AUTOMATIC = 0,
  MANUAL = 1
} mode_t;

mode_t mode = AUTOMATIC;

int outsideT = 0;
int outsideRH = 0;
int insideT = 0;
int insideRH = 0;

int disableFanTimer = 0;
int fanSpeed = 0;
int manualFanSpeed = 0;
int autoFanSpeed = 0;

void AnalogTemperatureRead( )
{
  char message[17];
  int t1 = ( analogRead( A1 ) * 125 ) >> 9;
  int t2 = ( analogRead( A2 ) * 125 ) >> 9;
  int t3 = ( analogRead( A3 ) * 125 ) >> 9;
  
  sprintf( message, "%d %d %d", t1, t2, t3);
  LCD_setCursor( 0,1 );
  LCD_print( message );
 
}

void setup() {
  
  ChipCapInit( );
  
  LCD_Init( );
  LCD_Clear( );

  // FAN speed control
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  
  // Analog temperature
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  
  // Heater flow sensor LOW = heater ON
  pinMode( 12, INPUT );
  
}

void UpdateFanSpeed( )
{
  char str[4];
  fanSpeed = ( mode == MANUAL ) ? manualFanSpeed : autoFanSpeed;
  if( disableFanTimer ) fanSpeed = 0;

  if( fanSpeed >= 0 && fanSpeed <= 15 )
  { 
    digitalWrite( 10, ( fanSpeed & 0x01 ) ? HIGH : LOW );
    digitalWrite( 11, ( fanSpeed & 0x02 ) ? HIGH : LOW );
    digitalWrite( 2, ( fanSpeed & 0x04 ) ? HIGH : LOW );
    digitalWrite( 3, ( fanSpeed & 0x08 ) ? HIGH : LOW );
 
    sprintf( str, "%02d", fanSpeed );
  }
  else strcpy( str, "**" );
  
  LCD_setCursor( 14,1 );
  LCD_print( str );  
}


void fastLoop( )
{
  if( digitalRead( 12 ) == LOW )
  {
    disableFanTimer = 10;
    UpdateFanSpeed( );
  }  
}

void loop100ms( )
{
  switch( LCD_ButtonTask( ))
  {
  case BUTTON_RIGHT:
    break;
  case BUTTON_LEFT:
    break;
  case BUTTON_UP:
    if( manualFanSpeed < 15 ) manualFanSpeed++;
    break;
  case BUTTON_DOWN:
    if( manualFanSpeed > 0 ) manualFanSpeed--;
    break;
  case BUTTON_SELECT:
    if( mode == MANUAL )
    {
      mode = AUTOMATIC;
    }
    else
    {
      mode = MANUAL;
    }
    break;
  }
  
  UpdateFanSpeed( );
  
  LCD_setCursor( 13,1 );
  if( mode == MANUAL ) LCD_print( "M" ); else LCD_print( "A" );  
}

void loop1sec( )
{
  char msg[17];
  ChipCapRead( 0x28, insideT, insideRH );
  ChipCapRead( 0x2A, outsideT, outsideRH );
  
  sprintf( msg, " In:%dC(%d%%)", insideT/10, insideRH );
  LCD_setCursor( 0,0 );
  LCD_print( msg );
  
  sprintf( msg, "Out:%dC(%d%%)", outsideT/10, outsideRH );
  LCD_setCursor( 0,1 );
  LCD_print( msg );
  
  if( disableFanTimer ) disableFanTimer--;
}

typedef struct {
  int t;
  int s;
} threshold_t;

const threshold_t THR[] = { {-40,0}, {8,4}, {10,8}, {12,10}, {14,12}, {16,15}, {25,11}, {30,5}, {60,0}};

#define SAMPLE_DURATION  3 // in 10 sec

void loop10sec( )
{
  int i;
  static int nextSampling = SAMPLE_DURATION;

/*
  char msg[3];
  static int n=0;  
  sprintf( msg, "%d", n++ );
  LCD_setCursor( 14,0 );
  LCD_print( msg );
*/

  if( disableFanTimer || mode != AUTOMATIC ) nextSampling = SAMPLE_DURATION;
  
  if( nextSampling <= SAMPLE_DURATION )
  {
    autoFanSpeed = 9;
    nextSampling--;
    // Next outside air sample will occur in 15 minutes if fan is running below step 6
    if( nextSampling <= 0 ) nextSampling = 15 * 6;
  }
  else
  {
    if( fanSpeed < 6 ) nextSampling--;
    
    /*if( calculateRH( ) > 60 ) autoFanSpeed = 4;*/
    
    for( i=0; i<sizeof(THR)/sizeof(THR[0]) - 1; i++ )
    {
      int t = round( outsideT / 10.0 );   
      if( t > THR[i].t && t < THR[i+1].t ) autoFanSpeed = THR[i].s;
    }
  }
  UpdateFanSpeed( );  
}

int calculateRH( )
{
  float t;
  float eOut;  
  float eIn;
  
  t = outsideT / 10.0;
  eOut = exp(( 17.625 * t ) / ( 243.04 + t ));
  t = 18.5;
  eIn = exp(( 17.625 * t ) / ( 243.04 + t ));
  t = outsideRH;
    
  return round( t * eOut / eIn );
}


void loop() {
  static int n = 0;
  static unsigned long nextLoop = 0;
  
  fastLoop( );
  
  if( millis( ) > nextLoop )
  {
    n++;
    loop100ms( );
    if(( n % 10 ) == 0) loop1sec( );
    if(( n % 100 ) == 0) loop10sec( );
    nextLoop = millis( ) + 100;
  }
      
  delay( 1 );
}
