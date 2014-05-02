#include "LCD.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"

#define LCD_D0  4
#define LCD_D1  5
#define LCD_D2  6
#define LCD_D3  7
#define LCD_RS  8
#define LCD_EN  9
#define LCD_BTN A0

void ShowError( int ErrNo, uint8_t code1, uint8_t code2, uint8_t code3 )
{
  char message[17];
  sprintf( message, "Err% 2d (%02X,%02X,%02X)", ErrNo, code1, code2, code3 );
  LCD_Clear( );
  LCD_setCursor( 0,0 );
  LCD_print( message );
  while( 1 );
}

void LCD_Init( )
{
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_EN, OUTPUT);
  pinMode(LCD_BTN, INPUT);
 
  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
  delayMicroseconds(50000); 
  // Now we pull both RS and R/W low to begin commands
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_EN, LOW);
   
  //put the LCD into 4 bit mode
 
  // this is according to the hitachi HD44780 datasheet
  // figure 24, pg 46
  pinMode(LCD_D0 , OUTPUT);
  pinMode(LCD_D1 , OUTPUT);
  pinMode(LCD_D2 , OUTPUT);
  pinMode(LCD_D3 , OUTPUT);

  // we start in 8bit mode, try to set 4 bit mode
  LCD_send_low(0x03);
  delayMicroseconds(4500); // wait min 4.1ms

  // second try
  LCD_send_low(0x03);
  delayMicroseconds(4500); // wait min 4.1ms
    
  // third go!
  LCD_send_low(0x03); 
  delayMicroseconds(150);

  // finally, set to 4-bit interface
  LCD_send_low(0x02);
  delayMicroseconds(5000);
 
  // finally, set # lines, font size, etc.
  LCD_command( LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS );
  delayMicroseconds(5000);
  
  LCD_command( LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF );
}

/********** high level commands, for the user! */
void LCD_Clear( void )
{
  LCD_command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  delayMicroseconds(5000);  // this command takes a long time!
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LCD_CreateChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  LCD_command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    LCD_write(charmap[i]);
  }
}

inline void LCD_command(uint8_t value) 
{
  LCD_send(value, LOW);
}

void LCD_setCursor(uint8_t col, uint8_t row)
{
  if( row != 0 ) row  = 0x40;
  LCD_send(LCD_SETDDRAMADDR | (col + row ), LOW );
}

void LCD_write(uint8_t value) {
  LCD_send(value, HIGH);
}

void LCD_send(uint8_t value, uint8_t mode) {
  LCD_send_high( value, mode );
  delayMicroseconds(500); 
  LCD_send_low( value );
  delayMicroseconds(500); 
}

// #####################################################################################
void LCD_setCursor_high(uint8_t col, uint8_t row)
{
  if( row != 0 ) row  = 0x40;
  LCD_send_high(LCD_SETDDRAMADDR | (col + row ), LOW );
}

void LCD_setCursor_low(uint8_t col, uint8_t row)
{
  if( row != 0 ) row  = 0x40;
  LCD_send_low(LCD_SETDDRAMADDR | (col + row ));
}

void LCD_write_high(uint8_t value) {
  LCD_send_high(value, HIGH);
}

void LCD_write_low(uint8_t value) {
  LCD_send_low(value);
}

void LCD_print( char* str )
{
  while( *str ) LCD_write( *str++ ); 
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void LCD_send_high(uint8_t value, uint8_t mode) {
  digitalWrite(LCD_RS, mode);
  digitalWrite(LCD_D0, value & 0x10);
  digitalWrite(LCD_D1, value & 0x20);
  digitalWrite(LCD_D2, value & 0x40);
  digitalWrite(LCD_D3, value & 0x80);
  LCD_pulseEnable();
}

void LCD_send_low( uint8_t value) {
  digitalWrite(LCD_D0, value & 0x01);
  digitalWrite(LCD_D1, value & 0x02);
  digitalWrite(LCD_D2, value & 0x04);
  digitalWrite(LCD_D3, value & 0x08);
  LCD_pulseEnable();
}

inline void LCD_pulseEnable(void) {
  digitalWrite(LCD_EN, LOW);
  digitalWrite(LCD_EN, HIGH);
  digitalWrite(LCD_EN, LOW);
}

// ADC readings expected for the 5 buttons on the ADC input
#define RIGHT_10BIT_ADC           0  // right
#define UP_10BIT_ADC            143  // up (136-5)
#define DOWN_10BIT_ADC          327  // down (311-5)
#define LEFT_10BIT_ADC          504  // left (479-5)
#define SELECT_10BIT_ADC        740  // right (703-5)
#define BUTTONHYSTERESIS         10  // hysteresis for valid button sensing window

LCD_Button LCD_ScanButtons( void )
{
  LCD_Button button = BUTTON_NONE;
  int buttonVoltage = analogRead( LCD_BTN );
  
  /*
  char tmp[6];
  sprintf( tmp, "%04d ", buttonVoltage );
  LCD_setCursor( 0,0 );
  LCD_print( tmp );
  */
  
   // Sense if the voltage falls within valid voltage windows
   if( buttonVoltage < ( RIGHT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_RIGHT;
   }
   else if(   buttonVoltage >= ( UP_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( UP_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_UP;
   }
   else if(   buttonVoltage >= ( DOWN_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( DOWN_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_DOWN;
   }
   else if(   buttonVoltage >= ( LEFT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( LEFT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_LEFT;
   }
   else if(   buttonVoltage >= ( SELECT_10BIT_ADC - BUTTONHYSTERESIS )
           && buttonVoltage <= ( SELECT_10BIT_ADC + BUTTONHYSTERESIS ) )
   {
      button = BUTTON_SELECT;
   }

   return button;
}

#define REPEAT_DELAY  750
#define CONT_DELAY    100
#define IDLE_DELAY    30

LCD_Button LCD_ButtonTask( )
{
  static LCD_Button prevButton = BUTTON_NONE;
  LCD_Button button = LCD_ScanButtons( );
  
  /*
  char str[6];
  sprintf( str, "% 4d", commandCount );
  LCD_SetStatus( str );
  
  char tmp[2];
  sprintf( tmp, "%d", state );
  LCD_SetStatus( tmp );
  */
  
  if( button != prevButton )
  {
    prevButton = button;
    return button;    
  }
  return BUTTON_NONE;  
}

