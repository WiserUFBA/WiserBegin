#include <stdint.h>
#include <string.h>

// Constants to connection with the broker
#define DEVICE_NAME   "base01"
#define MQTT_PORT     1883

// Pins used
#define LUMINOSITY    A3
#define SOM           A2
#define TEMPERATURE   6


//Hash that represents the attributes
#define H_temperatureSensor 0x5821FBAD
#define H_luminositySensor  0xF8D7A29C
#define H_lampActuator      0xEEF6732

#define int_T     0
#define float_T   1
#define str_T     2

#define sensorCount 2
#define actuatorCount 1

// Allow Software Serial 
//#define ENABLE_SOFTWARE_SERIAL

#define DEBUG_PORT ATMSerial
//#define DEBUG_PORT

// If enabled Software Serial
#ifdef ENABLE_SOFTWARE_SERIAL
#include <SoftwareSerial.h>
// Software Serial should be static since this file can be called multiple times
SoftwareSerial static ATMSerial(7,6);
#endif
#ifndef ENABLE_SOFTWARE_SERIAL
#define ATMSerial Serial
#endif
/*  This is a example sketch of a simple device who sends simple values to in response
    To another's device request.
    The objective is that he would be ables to...

    That device is in the scenario where he comunicates, through a serial port, 
    with another device.

    Each sensor(or any other funcionality like actuactors) has an associated ID
*/

// Variveis
int                 aux;
float               luz;
float               v_out;
volatile int        t;
volatile int        luminosity;
volatile int        flag = 1;
volatile int        state;
volatile int        state_relay;

byte ip[4] =        { 127, 0, 0, 1 };

//FLOW
unsigned long int lastTime;
char vector_response[60];

typedef struct sensorStruct{
  int id;
  uint8_t type;
}sensorStruct;

sensorStruct sensors[sensorCount];

typedef struct actuatorStruct{
  int id;
  uint8_t type;
}actuatorStruct;

actuatorStruct actuators[actuatorCount];

//  Sensors functions
/*<sensorFunctions>*/
int luminosity_sensor(int PIN){
  return analogRead(PIN);
}

float temperature_sensor(int PIN){
  return ((float(analogRead(PIN))*5/(1023))/0.01);
}
/*</sensorFunctions>*/

/*
  Sensor ID's defines
*/
//  <sensorsID>
#define ID_temp     0
#define ID_lumin    1
//  </sensorsID>

//  Actuators functions
/*<actuatorFunctions>*/

int lamp_actuator(int PIN, int state ){
  if(state == 0){
    digitalWrite(PIN, LOW);
    state_relay =  state;
    ATMSerial.println("Desligado");
  }
  else if(state == 2){
    state_relay =  !state_relay;
    digitalWrite(PIN, state_relay);
    state = state_relay;
    ATMSerial.println("Alternou");
  }
  else{
    digitalWrite(PIN, HIGH);
    state_relay =  state;
    ATMSerial.println("Ligado");
  }
}

/*</actuatorFunctions>*/
/*
  Actuators ID's defines
*/
//  <actuatorsID>
#define ID_lamp     0
//  </actuatorsID>

bool get(uint8_t id) {
  char response[15];
  double aux;

  switch(id){
    case ID_temp:
      // The dht_temperatures_sensor supports INFO and VALUE requests.
      aux = temperature_sensor(TEMPERATURE);
      break;
    case ID_lumin:
      // The lumisity_sensor supports INFO and VALUE,requests.
      aux = luminosity_sensor(LUMINOSITY);
      break;
    default:
      return false;
  }

  //treating the value type
  switch(sensors[id].type){
      case float_T:
        //<debug>
        ATMSerial.println("É float");
        dtostrf(aux,2,2,response);
        break;
      case int_T:
        //<debug>
        ATMSerial.println("É int");
        itoa((int)aux,response,10);
        break;
      default:
        //<debug>
        ATMSerial.println("É alguma coisa...");
        return false;
  }

  /*
    Sends the response to master
    <format> id:value:flag
    <example> response:
      (1) 9>65.34>s
        '9' is the id
        '65.34' is the value
        's' is the flag
      (2) 12>27>f
        '12' is the id
        '27' is the value
        'f' is the flag
    </example>
  */
  Serial.print(id);
  Serial.print(">");
  Serial.println(response);
  
  //<debug> Prints the message to master
  ATMSerial.print(id);
  ATMSerial.print(">");
  ATMSerial.println(response);
  //</debug>

  return true;
}

bool set(uint8_t id,int value) {
  ATMSerial.println("Setando");
  switch (id) {
    case ID_lamp:
      ATMSerial.println("Lamp!");
      break;
    default:
      return false;
  }
  return true;
}



void callback(char* buffer,int length){
  /*
    <example> request  :
      s:2
      's' is the flag(means 'sensor')
      '12' is the feature ID
    </example>
    So atoi(buffer[2] - 48) is the ID
    and buffer[0] is the flag
    
    <example> request  :
      a:1:2
      'a' is the flag(means 'actuator')
      '1' is the feature ID
      '2' is the request value
    </example>
    So atoi(buffer[2] - 48) is the ID
    and buffer[0] is the flag
    
  */
  switch(buffer[0]){
    case 's':
      get((buffer[2]-48));
      break;
    case 'a':
      set((buffer[2]-48),atoi(&buffer[4]));
      break;
    default:
      //<debug>
      ATMSerial.println("Erro...");
  }
  
}

void serial_read(char readch, byte *buffer, int len) {
  static int pos = 0;

  if (readch > 0) {
    switch (readch) {
      case '\n':
        /*Sends the message and its length(pos) to the callback function*/
        callback((char*)buffer, pos);
        pos = 0;
        break;
      default:
        if (pos < len - 1) {
          if (readch == '\r') return;
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
}
// SETUP
void setup() {

  Serial.begin(115200);
  ATMSerial.begin(115200);


  // type declarations
  /*<declarations>*/
  sensors[ID_temp].type  = float_T;
  sensors[ID_lumin].type = int_T;
  actuators[ID_lamp].type  = int_T;
  /*</declarations>*/

  ATMSerial.println("ATMEGA ready and waiting for instructions!");

}

// LOOP
void loop() {
  static byte buffer[100];
  serial_read(Serial.read(), buffer, 100);

}
