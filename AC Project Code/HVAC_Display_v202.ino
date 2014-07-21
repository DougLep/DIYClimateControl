//Doug Leppard version 2.00 start 2013 05 23
#define HVAC_version 2.00
/*
2.00 start of new version working version with I2c
works with HVAC_central 2.00
add i2c take out xbee coomunication between unos
slave i2c

*/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <SD.h>
#include <MemoryFree.h>  //report how much memory is available
#include <SoftwareSerial.h>  //communicate to xbee
#include <Wire.h>
SoftwareSerial xbeeSerial(2, 3); // RX, TX

#define SD_CS 5 // Card select for shield use

Adafruit_TFTLCD tft;
uint8_t         spi_saveSD;  //to save spi for SD
uint8_t         spi_saveDisplay;  //to save spi for SD

// These are the pins for the shield!
#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin
 
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940
 
 // For better pressure precision, we need to know the resistance
 // between X+ and X- Use any multimeter to read it
 // For the one we're using, its 300 ohms across the X plate
 //note messured it and it was 394
//setcursoi max x is 300, max y is 230, 

 TouchScreen ts = TouchScreen(XP, YP, XM, YM, 394);
 
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
 
 // Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
 
#define BOXSIZE   40
#define PENRADIUS  4
 
#define MINPRESSURE 10
#define MAXPRESSURE 1000
 
#define EOL 13  //end of line CR dec 13
 
unsigned long refresh_time = 0;  //count how lont to refresh
boolean in_menu = false;        //tell if it is in a menu or not

#define max_print_buffer 200
char print_buffer[max_print_buffer]; //holds incoming data from i2c
  //note if buffer_load = buffer print then no char are avaiable to print
byte buffer_load = 0;  //next place to be loaded with char
byte buffer_print  = 0;  //next char to be printed

//------------ setup ----------------------
void setup(void) 
{
  Serial.begin(9600);  //computer output
  progmemPrint(PSTR("HVAC_Display "));
  Serial.println(HVAC_version);
  print_free_memory();
  
  xbeeSerial.begin(9600);  //xbee serial setup

  Wire.begin(9);                // Start I2C Bus as a Slave (Device Number 9)
  Wire.onReceive(receiveEvent); // register event
  
  tft.reset();
  uint16_t identifier = tft.readID();
  tft.begin(identifier);  //????
  blank_screen();  //blank screen say hello
}

//----------- loop -------------------------
void loop()
{
  if (in_menu)
  {
    char option = check_touchpad();  //now have option
    switch (option) {
      case 'N':
        //nothing pressed so keep trying
        break;
      case '1':
Serial.print("case 1");
        print_log;
xbeeSerial.println("print log");
        break;
      case 'E':  //exit menu
        in_menu = false;  //return from menu
        blank_screen();  //blank screen say hello
        break;        
      default:
        in_menu = false;  //return from menu
    }
  }
  else
  {  //not in menu but display mode
    if (buffer_print != buffer_load) //if not = then must print
    {
      process_buffer();  //print wht is in buffer
    }
    if (check_touchpad() != 'N')  //check if touch pad has been pressed
    {
        blank_screen();  //blank screen say hello
        put_menu_up();
        in_menu = true;
    }
  }
}

//-------------put_menu_up--------------------
void put_menu_up()
{
  tft.fillScreen(BLACK);
  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, RED);
  tft.fillRect(0,BOXSIZE, BOXSIZE, BOXSIZE, YELLOW);
  tft.fillRect(0,BOXSIZE*2,BOXSIZE, BOXSIZE, GREEN);
  tft.fillRect(0,BOXSIZE*3,BOXSIZE, BOXSIZE, CYAN);
  tft.fillRect(0,BOXSIZE*4,BOXSIZE, BOXSIZE, BLUE);
  tft.fillRect(0,BOXSIZE*5,BOXSIZE, BOXSIZE, MAGENTA);
  tft.setTextSize(2);  //1 default, 2 is almost too large
  tft.setCursor(60,10);
  progmemPrinttft(PSTR("Print Log"));
  tft.setCursor(60,50);
  progmemPrinttft(PSTR("2"));
  tft.setCursor(60,90);
  progmemPrinttft(PSTR("3"));
  tft.setCursor(60,130);
  progmemPrinttft(PSTR("4"));
  tft.setCursor(60,170);
  progmemPrinttft(PSTR("5"));
  tft.setCursor(60,210);
  progmemPrinttft(PSTR("Exit Menu "));
  
 

}

//---------- print log ----------
void print_log()
{
  Serial.println("print log");
  xbeeSerial.println("print log");
}

//-------------- check_touchpad ----------------------
char check_touchpad()  //check if touch pad has been pressed
{
/*
  for menu use these coordinates
  red pX 191	pY 159	Pressure = 329
  207
  
  yellow 182 249 423
  312
  
  green 200 375 231
  430
  
  lite blue 210 472
  522
  
  dark blue 221 604 582
  640
  
  purple 200 705 311
  754
*/
  char pressed = 'N';  //say not pressed
  //pressure point
  Point p = ts.getPoint();  //get pressure point

  //???? don't now what this section is ------------------------
  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);
 
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) //see if keypressed
  {
    progmemPrint(PSTR("pX ")); Serial.print(p.x);
    progmemPrint(PSTR("\tpY ")); Serial.print(p.y);
    progmemPrint(PSTR("\tPressure = ")); Serial.println(p.z);
    pressed = 'Y';  //say was pressed, add menu later
    if (p.y < 207)
    {
      pressed = '1';  //option 1
    }
      else
    {
      if (p.y < 312)
      {
        pressed = '2';  //option 2
      }
      else
      {
        if (p.y < 430)
        {
          pressed = '3';  //option 3
        }
        else
        {
          if (p.y < 522)
          {
            pressed = '4';  //option 4
          }
          else
          {
            if (p.y < 640)
            {
              pressed = '5';  //option 5
            }
            else
            {
              pressed = 'E';  //exit menu
            }
          }
        }
      }
    }

    delay(100); //for debounce
  }
  if (pressed != 'N')
  {
    progmemPrint(PSTR("option "));
    Serial.println(pressed);
  }
  return pressed;
}

//-------------- blank screen --------------------
void blank_screen()
//blank screen say hello
{
  tft.fillScreen(BLACK);  //FILL SCREEN WITH BLACK
  tft.setRotation(2);  //puts it in same roation as touch screen, 
  //top opposite of usb plug in
  tft.setCursor(0, 0);
  tft.setTextSize(1);  //1 default, 2 is almost too large
  progmemPrinttft(PSTR("HVAC_Display "));
  tft.println(HVAC_version);
  tft.println();
}

//----------------progmemPrint-----------------------------------------
// Copy string from flash to serial port of nss
// Source string MUST be inside a PSTR() declaration!
void progmemPrint(const char *str) {
  char c;
  while(c = pgm_read_byte(str++)) Serial.print(c);
}

// Same as above, with trailing newline
void progmemPrintln(const char *str) {
  progmemPrint(str);
  Serial.println();
}

//----------------progmemPrinttft-----------------------------------------
// Copy string from flash to serial port of nss
// Source string MUST be inside a PSTR() declaration!
void progmemPrinttft(const char *str) {
  char c;
  while(c = pgm_read_byte(str++)) tft.print(c);
}

// Same as above, with trailing newline
void progmemPrintlntft(const char *str) {
  progmemPrint(str);
  tft.println();
}

//-------------------------------------------
void print_free_memory()
{
  progmemPrint(PSTR("freeMemory()="));
  Serial.println(freeMemory());
}

//---------------- receiveEvent ---------------
void receiveEvent(int howMany) //receives data from i2c HVAC_Central
{
    //from HVAC_central
//Serial.print("-");   Serial.print(buffer_load); Serial.print(buffer_print); 
  while(0 < Wire.available()) // loop through all 
  {
    print_buffer[buffer_load] = Wire.read(); // receive byte as a character
    buffer_load++;
    if (buffer_load > max_print_buffer-1)  //should never reach max
    {
      buffer_load = 0;
    }
  }
}

//------------------ process_buffer ------------------
void process_buffer()  //process what is in buffer
{
//Serial.print("+");
  if (in_menu == false)  //if it is in menu ignore
  {
    while (buffer_print != buffer_load)  //print all that is in buffer
    {
      char ch = print_buffer[buffer_print];
      buffer_print++;
      if (buffer_print > max_print_buffer-1)
      {
        buffer_print = 0;
      }
      if (ch == '~')
      {
        blank_screen();
        Serial.println("blank screen");
      }
      else
      {
        tft.print(ch);
        Serial.print(ch);
      }
    }
  }   
}

