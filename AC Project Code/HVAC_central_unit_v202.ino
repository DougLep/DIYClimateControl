//this controls everything, it is the central unit to AC project
//Doug Leppard
  #define HVAC_version 2.02
/*
2.02 based on 2.00
Experimental
adding to control HVAC with one temp sensor, only 1st stage
2.00 working version with I2c
logs and displays data all sensors located
works with HVAC_display 2.00
add i2c take out xbee coomunication between unos
master i2c

*/

  #include <XBee.h> 
  #include <MemoryFree.h>  //report how much memory is available
  #include <SD.h>
  #include <EEPROM.h>    //can write to eeprom
  #include <Wire.h>
  #include "RTClib.h"

/* this section taken out because not using 2nd xbee
  #include <SoftwareSerial.h>
     This example is for Series 2 (ZigBee) XBee Radios only Receives 
     I/O samples from a remote radio. The remote radio must have IR > 0 
     and at least one digital or analog input enabled. The XBee coordinator
     should be connected to the Arduino.   This example uses the SoftSerial
     library to view the XBee communication.  I am using a  Modern Device
     USB BUB board (http://moderndevice.com/connect) and viewing the output
     with the Arduino Serial Monitor. 
  // Define NewSoftSerial TX/RX pins
  // Connect Arduino pin 8 to TX of usb-serial device 
  uint8_t ssRX = 8;
  // Connect Arduino pin 9 to RX of usb-serial device 
  uint8_t ssTX = 9;
  // Remember to connect all devices to a common Ground: XBee, Arduino and USB-Serial device
  //NOTE this used to be communication between UNOs, now I2c does it
//  SoftwareSerial nss(ssRX, ssTX);
*/

  XBee xbee = XBee();

  ZBRxIoSampleResponse ioSample = ZBRxIoSampleResponse(); 

  XBeeAddress64 test = XBeeAddress64();  
  
  RTC_DS1307 RTC;
  File dataFile;  //logger file
  #define SD_CS 10 // Card select for shield use
  //note port 11 and 12 used with SD card

  // Pin 13 has an LED connected on most Arduino boards.
  // give it a name:
  #define led  13

//HVAC controls
  #define stage1 5
  #define stage2 6
  #define fan 7
  #define uptemp 2  //button to make set temperature to go up
  #define downtemp 3  //button to make set temperature to go down
  
    
  #define EOL 13  //end of line CR dec 13

  //------------- global varable section ----------------
  #define CR          13
  
  unsigned long time;    //used to say what time
  #define max_sensors 15  //max number of sensors
  #define no_eeprom_char 26 //number chars per xbee record in eeprom

  byte sensor_port[max_sensors];  //what port is reporting
  byte sensor_type[max_sensors];  //type of sensor
  byte number_sensors = 0;  //number of active sensors
  uint32_t sensor_serial[max_sensors];  //sensor serial number
//  char sensor_name[max_sensors][10];  //name of the sensor
  float sensor_temperature[max_sensors];  //sensor temp
  float adjust_temperature[max_sensors];  //used to store correction of temps
  unsigned int last_report[max_sensors];  //last time this sensor ws reported in seconds
  byte sensor_on;                        //sensor just found
  float set_temp = 77.00;        //temperature set at
  
  uint32_t current_serial;      //current sensor serial number that is being processed
  
  unsigned long alive_time = 0;      //used for testing
  unsigned long log_time = 0;      //used for logging

  boolean toggle = true;         //used for testing
  
  //-------------- setup ------------------------------
void setup() {

  //Serial works with xbee 2001 and will dumpt to terminal
  //progmemPrint goes to serial
  //progmemPrinti2c goes to i2c
  
  Serial.begin(9600);  //used to connect to sensor xbee
  Serial.println(freeMemory());

  xbee.setSerial(Serial); 
  // start soft serial   
  //now i2c does this
//  nss.begin(9600);  //communicates to display unit or others in at mode
  progmemPrint(PSTR("HVAC_central_unit "));
  Serial.println(HVAC_version);
  pinMode(led, OUTPUT);     
  //control HVAC
  pinMode(stage1, OUTPUT);     
  pinMode(stage2, OUTPUT);     
  pinMode(fan, OUTPUT);
  pinMode(uptemp, INPUT_PULLUP);
  pinMode(downtemp, INPUT_PULLUP);

  //test
//  digitalWrite(stage1, LOW);
//  digitalWrite(stage2, LOW);
//  digitalWrite(fan, LOW);
//delay(500);  

  //start with all off  
  digitalWrite(stage1, HIGH);
  digitalWrite(stage2, HIGH);
  digitalWrite(fan, HIGH);
  
  print_free_memory();
  Wire.begin(); // Start I2C Bus as Master
  progmemPrintlni2c(PSTR("HVAC_central_unit"));
  RTC.begin();
 
  progmemPrint(PSTR("SD card "));
  if (!SD.begin(SD_CS)) {
    progmemPrintln(PSTR("SD failed!"));
//    progmemPrintlnnss(PSTR("SD failed!"));
    while(1);  //halt program
  } 

  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (! dataFile) {
    progmemPrintln(PSTR("error opening datalog.txt"));
//    progmemPrintlnnss(PSTR("error opening datalog.txt"));
    while (1) ;
  }
  progmemPrintln(PSTR("opened datalog.txt"));

  get_eeprom_data();  //load from eeprom data into varables
  alive_time =  millis();
}

//------------------------ loop -----------------------------
void loop() 
{ 

    //check push buttons to see if need temperature change
  if (digitalRead(uptemp) == LOW)  //check button if pushed to raise temp
  {
    delay(50);    //make sure button not bouncing
    set_temp = set_temp +0.50;
    alive_time = 0;  //force it to report temperature
    while (digitalRead(uptemp) == LOW)
    {
       //do nothing waiting for person to raise button
    }
  }
  if (digitalRead(downtemp) == LOW)  //check button if pushed to lower temp
  {
    delay(50);    //make sure button not bouncing
    set_temp = set_temp -0.50;
    alive_time = 0;  //force it to report temperature
    while (digitalRead(downtemp) == LOW)
    {
       //do nothing waiting for person to raise button
    }
  }

  //----------send report to display------------------------------------------------
  if (millis() > alive_time +5100)  //15.1 sec report
  {
Serial.println(millis()/1000);
    Wire.beginTransmission(9); // transmit to device #9 UNO display
    Serial.println(freeMemory());
    progmemPrinti2c(PSTR("~"));  //tell to blank screen
    delay(100);      //give it some time
    alive_time =  millis();
    for (int ii=0; ii<number_sensors; ii++)
    {
      //get name of room
      for (int char_pos =0; char_pos<10; char_pos++)
      {
        char eeprom_char = get_eeprom(ii,char_pos);
        printchari2c(eeprom_char);
      }
      progmemPrinti2c(PSTR(" "));
      //temperature
      float report_temp = sensor_temperature[ii] + adjust_temperature[ii];
      if (report_temp >20)  //report if within range else skip
      {
        if ((report_temp <10) and (report_temp>0))  //give spacing for number of digits
        {
          progmemPrinti2c(PSTR(" "));
        }
        printfloati2c(report_temp);
        //last report
        progmemPrinti2c(PSTR(" "));
        unsigned int time_lapse = millis()/1000 - last_report[ii];   //calculate long long sense sensor reported
Serial.println(time_lapse);
        if (time_lapse < 10)  //pad for single digit
        {
          progmemPrinti2c(PSTR(" "));
        }
        if (time_lapse>99)  //keep it to no more than 99 for better printing
        {
          time_lapse = 99;
        }
        printinti2c(time_lapse);
      }
      printlni2c();
    }
    printlni2c();
    printlni2c();
    progmemPrinti2c(PSTR("Set Temperature "));
    printfloati2c(set_temp);
    printlni2c();

Serial.println("report data");

    set_HVAC();  //set the HVAC power settings
  }
  

  //-------------- log data ----------------------------------------
  if (millis() > log_time + 60000)  //log once per minute
  {
progmemPrinti2c(PSTR("log data"));
printlni2c();
    log_time = millis();  //setup for next report

    //write time
    DateTime now = RTC.now();
    dataFile.print(now.year(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.day(), DEC);
    dataFile.print(' ');
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print("\t");  //print tab
    //write sensor data
    for (int ii=0; ii<number_sensors; ii++)
    {
      //get name of room and write it out
      for (int char_pos =0; char_pos<10; char_pos++)
      {
        char eeprom_char = get_eeprom(ii,char_pos);
        dataFile.print(eeprom_char);
      }
      dataFile.print("\t");  //print tab
      //get adjusted temperature and write it out
      float report_temp = sensor_temperature[ii] + adjust_temperature[ii];
      dataFile.print(report_temp);
      dataFile.print("\t");  //print tab
    }
    dataFile.println();  //dine with line give return
    dataFile.flush();  //save the data, slows things up but safer if looses power
  }

  //---------------- read packets from xbee sensors
  //attempt to read a packet    
  xbee.readPacket(); 
  if (xbee.getResponse().isAvailable()) {  
    // got something 
Serial.print("got data ");
    if (xbee.getResponse().getApiId() == ZB_IO_SAMPLE_RESPONSE) 
    { 
      xbee.getResponse().getZBRxIoSampleResponse(ioSample); 
      current_serial = ioSample.getRemoteAddress64().getLsb();  //load serial
      // read analog inputs 
      for (int i = 0; i <= 4; i++) {  
        if (ioSample.isAnalogEnabled(i)) 
        {
          //found active address and port
          //find on list and record temp
          sensor_on=0;
          while (sensor_on<number_sensors)
          {
            if (sensor_serial[sensor_on] == current_serial)  //sensor serial number
            {
              if (sensor_port[sensor_on] == i)
              {
                break;
              }
            }
            sensor_on++;
          }
          // calculate temperature from TMP 36
          // 1.2V/ 1024 (10 bit ADC), 0°C is 500 mV, 10mV/ °C temperature coefficient
          float temperatureC = (((1.171875*ioSample.getAnalog(i))-500)/10);
          float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0; 
          sensor_temperature[sensor_on] = temperatureF;
          last_report[sensor_on] = millis()/1000;  //record when reported
 Serial.println(sensor_on);
       }
      }
    }
  }
}

//---------- end Loop -------------------------------------------

//----------------- control HVAC unit --------------------
//this will allow testing of progrram with one temp sensor and using only stage 1
void set_HVAC()
{
  Serial.println("set hvac");

  //this just uses kitchen temp which is 2 sensor
  float report_temp = sensor_temperature[2] + adjust_temperature[2];
  if (report_temp > set_temp + 0.30)
  {
    digitalWrite(stage1, LOW);
    digitalWrite(fan, LOW);
  }    
  if (report_temp < set_temp - 0.30)
  {
    digitalWrite(stage1, HIGH);
    digitalWrite(fan, HIGH);
  }
}

//-------------------------------------------
void print_free_memory()
{
  progmemPrint(PSTR("freeMemory()="));
  Serial.println(freeMemory());
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

//----------------progmemPrinti2c-----------------------------------------
// Copy string from flash to serial port of nss
// Source string MUST be inside a PSTR() declaration!
void progmemPrinti2c(const char *str) {
  Wire.beginTransmission(9); // transmit to device #9
  char c;
  while(c = pgm_read_byte(str++)) 
  {
    Wire.print(c);
//Serial.print(c);
  }    
  Wire.endTransmission();    // stop transmitting
}

// Same as above, with trailing newline
void progmemPrintlni2c(const char *str) {
  progmemPrinti2c(str);
  Wire.beginTransmission(9); // transmit to device #9
  Wire.println();
//Serial.println();
  Wire.endTransmission();    // stop transmitting
}

//----------printlni2c--------------------
void printlni2c()  //gives a cr lf
{
  Wire.beginTransmission(9); // transmit to device #9
  Wire.println();
//Serial.println();
  Wire.endTransmission();    // stop transmitting
}

//---------------printfloati2c--------------
void printfloati2c(float f)  //print a single ascii char
{
  Wire.beginTransmission(9); // transmit to device #9
  Wire.print(f);
//Serial.print(f);
  Wire.endTransmission();    // stop transmitting
}

//---------------printinti2c----------------
void printinti2c(int i)  //print a single ascii char
{
  Wire.beginTransmission(9); // transmit to device #9
  Wire.print(i);
//Serial.print(i);
  Wire.endTransmission();    // stop transmitting
}

//---------------printchari2c----------------
void printchari2c(char c)  //print a single ascii char
{
  Wire.beginTransmission(9); // transmit to device #9
  Wire.print(c);
//Serial.print(c);
  Wire.endTransmission();    // stop transmitting
}

//------------ eeprom data -------------------------
void get_eeprom_data()  //load from eeprom data into varables data on sensors
{
  int eeprom_count = 0;  //reset at bigging of eeprom
  progmemPrint(PSTR("# sensors "));
  int number = EEPROM.read(eeprom_count);
  eeprom_count++;
  number_sensors = number*10;   //this was tens place
  number = EEPROM.read(eeprom_count);
  number_sensors = number_sensors + number;  //ones place
  eeprom_count++;
  Serial.println(number_sensors);
  //xbee info
  for (int s = 0; s<number_sensors; s++)
  {    
    last_report[s]=0;  //preset it to 0 for report
    sensor_temperature[s]=0;  //preset it to 0 for report
    //serial numbers
    sensor_serial[s]=0;  //start with 0 and load each hex for sensor serial number
    for (int ss = 0; ss<8; ss++)
    {
      char c = EEPROM.read(eeprom_count);
      int ii = hextoi(c);
      //sensor serial number
      sensor_serial[s] = sensor_serial[s] << 4;  //shift 4 left
      sensor_serial[s] = sensor_serial[s] + ii;  //add the next hex
      eeprom_count++;
    }
    Serial.print(sensor_serial[s], HEX);
    progmemPrint(PSTR(" "));
    //port numbers
    byte c = EEPROM.read(eeprom_count);
    sensor_port[s] = c;
    Serial.print(c);
    eeprom_count++;
    progmemPrint(PSTR(" "));
    //type sensor
    c = EEPROM.read(eeprom_count);
    sensor_type[s] = c;
    Serial.print(c);
    eeprom_count++;
    progmemPrint(PSTR(" *"));
    //zone name
    for (int ss = 0; ss<10; ss++)
    {
      char c = EEPROM.read(eeprom_count);
      Serial.print(c);
      eeprom_count++;
    }
    progmemPrint(PSTR("* "));
    //temperature adjust
    c = EEPROM.read(eeprom_count);
    eeprom_count++;
    boolean minus = false;
    if (c == '-')
    {
      minus = true;
    }
    adjust_temperature[s] = 0;  //start at 0 and read data
    //ones place
    c = EEPROM.read(eeprom_count);
    eeprom_count++;
    adjust_temperature[s] = c - 48;  //48 for zero
    //decimal ignore
    c = EEPROM.read(eeprom_count);
    eeprom_count++;
    //1/10 place
    c = EEPROM.read(eeprom_count);
    adjust_temperature[s] = adjust_temperature[s] + (c-48)/10.0;
    eeprom_count++;
    //1/100 place
    c = EEPROM.read(eeprom_count);
    adjust_temperature[s] = adjust_temperature[s] + (c-48)/100.0;
    if (minus)
    {
      adjust_temperature[s] = -adjust_temperature[s];
    }

    eeprom_count++;
    progmemPrint(PSTR(" "));
    Serial.print(adjust_temperature[s]);
    Serial.println();
  }
}  

//----------------- hextoi ---------------------------------------
//convert hex ascii to int number
int hextoi(char h)  //take hex ascii char and convert to int
{
  int ih = 0;  //start 0
  if ( h >= 'A' )  //if it is an A-F
  {
    ih = h -'A'+ 10;
  }
    else
  {
    ih = h - '0';  //else it is 0-9 use this
  }
  return ih;  
}

//------------------ read from eeprom name of room -----------------
char get_eeprom(int sensor, int char_pos)
//this will get from eeprom name of sensor, sensor on and char on
{
  //2 is fisrt two bytes, sensor count, 10 is the 10th position in record
  int eeprom_pos = 2 + (sensor*(no_eeprom_char-1)) + char_pos + 10;
  return EEPROM.read(eeprom_pos);
}




  
