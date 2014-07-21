//this sets the eeprom on the central unit
//Doug Leppard
  #define HVAC_version 1.01
/*
1.01 first setup see HVAC setup doc for info
loaded with data 2012 05 16

*/
  #include <EEPROM.h>    //can write to eeprom
  int eeprom_count =0;  //what byte we are on in prom
  int number;          //used for calculations
  #define number_sensors  10 //number of sensors loading
  const char* low_serial[][number_sensors] = {
                                       {"408B429C"},
                                       {"408B42AA"},
                                       {"408B4296"},
                                       {"408B42B7"},
                                       {"408B4293"},
                                       {"408B42C3"},
                                       {"408B4294"},
                                       {"408B4294"},
                                       {"408B429E"},
                                       {"40902566"}
                                        };
   char low_serial_set[8];  //used with above
   byte port[number_sensors] =        {0,0,0,0,0,0,0,1,0,0};  //port used on sensor
   byte type_sensor[number_sensors] = {0,0,0,0,0,0,1,2,0,0};  //
   const char* zone_name[][number_sensors] = {
                                       {"Room 201  "},
                                       {"Room 203  "},
                                       {"Kitchen   "},
                                       {"Family RM "},
                                       {"Master    "},
                                       {"Room 202  "},
                                       {"AC out    "},
                                       {"AC in     "},
                                       {"AC thermo "},
                                       {"Room 204  "},
                                        };
   char zone_name_set[10];  //used with above
   const char* temp_adjust[][number_sensors] = {  //adjust temperature
                                       {" 1.13"},
                                       {"-2.36"},
                                       {"-2.83"},
                                       {" 1.64"},
                                       {"-2.60"},
                                       {" 0.16"},
                                       {" 3.96"},
                                       {" 1.85"},
                                       {" 2.67"},
                                       {"-1.86"},
                                        };
   char temp_adjust_set[10];  //used with above
   
   
  //-------------- setup ------------------------------
void setup() {
  Serial.begin(9600);  //used to connect to sensor xbee
  //not sure what is the difference between serial and nss
  Serial.print("HVAC_central_EEprom ");
  Serial.println(HVAC_version);
  delay(1000);
}

void loop() { 
  //now load sensors then stop
  //number of sensors
  if (number_sensors > 9 )
  {
    number = number_sensors/10;
    write_eeprom_number(number);
  }  else
  {
    number = 0;
    write_eeprom_number(number);
  }
  //now second char
  number = number_sensors -10*number;  //have least significan number
  write_eeprom_number(number);
  //write serial number
  for (int s = 0; s<number_sensors; s++)
  {
    const char** low_serial_set = low_serial[s];
    for (int ss = 0; ss<8; ss++)
    {
      char c = low_serial_set[0][ss];
      EEPROM.write(eeprom_count,c);
      eeprom_count++;
    }
    //write port numbers
    EEPROM.write(eeprom_count,port[s]);
    eeprom_count++;
    EEPROM.write(eeprom_count,type_sensor[s]);
    eeprom_count++;
    //zone name
    const char** zone_name_set = zone_name[s];
    for (int ss = 0; ss<10; ss++)
    {
      char c = zone_name_set[0][ss];
      EEPROM.write(eeprom_count,c);
      eeprom_count++;
    }
    //temperature adjust
    const char** temp_adjust_set = temp_adjust[s];
    for (int ss = 0; ss<5; ss++)
    {
      char c = temp_adjust_set[0][ss];
      EEPROM.write(eeprom_count,c);
      eeprom_count++;
    }
    
  }  
 
  //now report back what is in eeprom
  eeprom_count = 0;  //reset at bigging of eeprom
  Serial.println();
  Serial.print("number sensors = ");
  number = EEPROM.read(eeprom_count);
  Serial.print(number);
  eeprom_count++;
  number = EEPROM.read(eeprom_count);
  eeprom_count++;
  Serial.println(number);
  //xbee info
  for (int s = 0; s<number_sensors; s++)
  {    
    //serial numbers
    for (int ss = 0; ss<8; ss++)
    {
      char c = EEPROM.read(eeprom_count);
      Serial.print(c);
      eeprom_count++;
    }
    Serial.print(" ");
    //port numbers
    byte c = EEPROM.read(eeprom_count);
    Serial.print(c);
    eeprom_count++;
    Serial.print(" ");
    //type sensor
    c = EEPROM.read(eeprom_count);
    Serial.print(c);
    eeprom_count++;
    Serial.print(" *");
    //zone name
    for (int ss = 0; ss<10; ss++)
    {
      char c = EEPROM.read(eeprom_count);
      Serial.print(c);
      eeprom_count++;
    }
    Serial.print("* ");
    //temperature adjust
    c = EEPROM.read(eeprom_count);
    eeprom_count++;
    boolean minus = false;
    if (c == '-')
    {
      minus = true;
    }
    float temperature_adjust = 0;
    //ones place
    c = EEPROM.read(eeprom_count);
Serial.print(c);
    eeprom_count++;
    temperature_adjust = c - 48;  //48 for zero
    //dec ignore
    c = EEPROM.read(eeprom_count);
Serial.print(c);
    eeprom_count++;
    //1/10 place
    c = EEPROM.read(eeprom_count);
    temperature_adjust = temperature_adjust + (c-48)/10.0;
Serial.print(c);
    eeprom_count++;
    //1/100 place
    c = EEPROM.read(eeprom_count);
    temperature_adjust = temperature_adjust + (c-48)/100.0;
    if (minus)
    {
      temperature_adjust = -temperature_adjust;
    }

    eeprom_count++;
    Serial.print(" ");
    Serial.print(temperature_adjust);
 
    Serial.println();
  }
  

  // all done  
  while (1);  //put in loop
}

void write_eeprom_number(int write_number)
{
  EEPROM.write(eeprom_count,write_number);
  eeprom_count++;
}  
