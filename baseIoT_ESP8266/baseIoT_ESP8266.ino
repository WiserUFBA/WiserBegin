#include <ESP8266WiFi.h>
#include <pgmspace.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <TATUDevice.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <FlowController.h>

//workaround
byte ip[5] = "11";
//
char vector_response[1024];

// Update these with values suitable for your network.
const char*   device_name     = "base01";
const char*   mqtt_user       = "wiser";
const char*   mqtt_pass       = "wiser2014";
const int     mqttport        = 1883;

// Constants to connection with the broker
const char*   ssid            = "wiser";          
const char*   password        = "wiser2014";      
const char*   mqtt_server     = "192.168.0.120";  
char          subsc_topic[20] = "dev/";

bool get(uint32_t hash, void* response, uint8_t code);
bool set(uint32_t hash, uint8_t code, void* response);

// System variables
WiFiClient    espClient;
TATUInterpreter interpreter;
MQTT_BRIDGE(bridge);
TATUDevice device(device_name, ip, 121, 88, 0, mqttport, 1, &interpreter, get, set, bridge);
MQTT_CALLBACK(bridge, device, mqtt_callback);
PubSubClient  client(espClient);
MQTT_PUBLISH(bridge, client);

FlowController fluxo(&device, vector_response);
FlowUnit unit1, unit2, unit3, unit4;

bool flow(uint32_t hash, uint8_t code, void* response) {
  fluxo.flowbuilder((char*)response, hash, code);
  return true;
}

const char set_flow[] = "FLOW INFO temperatureSensor {\"collect\":400,\"publish\":2000}";
const char set_f_luminosity[] = "FLOW INFO luminositySensor {\"collect\":400,\"publish\":2000}";
int buffer_int_flow[100];
//char buffer_char_flow[100][10];
char buffer_char_flow[100][10];
char buffer_char_flow2[100][10];

/*
  Sensor ID's defines
*/
//  <sensorsID>
#define ID_temp     0
#define ID_lumin    1
//  </sensorsID>

/*
  Actuators ID's defines
*/
//  <actuatorsID>
#define ID_lamp     0
//  </actuatorsID>

//Hash that represents the attributes
#define H_temperatureSensor 0x5821FBAD
#define H_luminositySensor  0xF8D7A29C
#define H_lampActuator      0xEEF6732
//flow
#define H_flow              0x7C96D85D




bool get(uint32_t hash, void* response, uint8_t code) {
  uint8_t id;
  switch (hash) {
    case H_temperatureSensor:
      // The dht_temperatures_sensor supports INFO and VALUE requests.
      id = ID_temp;
      break;
    case H_luminositySensor:
      // The lumisity_sensor supports INFO and VALUE,requests.
      id = ID_lumin;
      break;
    case H_flow:
      STOS(response, vector_response);
      return true;
      break;
    default:
      return false;
  }

  //<bus>
  char bus_buffer[20];
  requestSensor(id, 's');
  //read_serial((int*)response,id);
  double bus_res;
  read_serial(&bus_res, id);
  //Handling types
  //int
  if (code == TATU_CODE_VALUE) {
    //*(int*)response = atoi(bus_buffer);
    *(int*)response = bus_res;
    //ESPSerial.print("Response: ");
    //ESPSerial.println(*(int*)response);
  }
  //</bus>

  return true;
}

bool set(uint32_t hash, uint8_t code, void* response) {
  uint8_t id;
  switch (hash) {
    case H_lampActuator:
      id = ID_lamp;
      break;
    default:
      return false;
  }
  
  //<bus>
  //Handling types
  //int
  if (code == TATU_CODE_VALUE) {
    //*(int*)response = atoi(bus_buffer);
    requestActuator(id, 'a',(int)response);
  }
  //</bus>

  return true;
}

void setup_wifi() {

  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
}

void requestSensor(int id, uint8_t type) {
  char req[8];
  sprintf(req, "%c:%d", type, id);
  Serial.println(req);
}

void requestActuator(int id, uint8_t type, int value) {
  char req[8];
  sprintf(req, "%c:%d:%d", type, id, value);
  Serial.println(req);
}

//Predicado que verifica se o id de retorno está correto
bool pred_id(char* topic, int id) {
  int count = 0;
  if ((atoi(topic) != id)) {
    ESPSerial.println("Mensagem Errada!");
    while (!Serial.available() && count < 10) {
      count++;
      delay(50);
    }
    return true;
  }

  return false;
}

void read_serial(double* response, int id) {
  int i;
  int count = 0;
  char msg[255];
  char topic[20];

  do {
    count++;
    if (count > 6) break;

    delay(50);
    ESPSerial.println("ATMEGA Response");
    ESPSerial.print("Topic = ");
    for (i = 0; Serial.available(); i++ ) {
      topic[i] = Serial.read();
      if (topic[i] == '>') break;
      ESPSerial.print(topic[i]);
    }
    ESPSerial.println();

    topic[i] = 0;

    ESPSerial.print("Message = ");
    for (i = 0; Serial.available(); i++) {
      msg[i] = Serial.read();
      ESPSerial.print(msg[i]);
      if (msg[i] == '\n') break;
    }

    msg[i] = 0;

    //fail safety
  } while (pred_id(topic, id));

  //int
  *response = atof(msg);
  ESPSerial.print("Response: ");
  ESPSerial.println(*response);
  //strcpy(response,msg);

}

void reconnect() {
 
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect(device_name, mqtt_user, mqtt_pass)) {
      // Once connected, publish an announcement...
      client.publish("CONNECTED", device_name);
      // ... and resubscribe
      client.subscribe("dev/");
      client.subscribe(subsc_topic);
    }
    else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

}

void setup() {

  Serial.begin(115200);
  ESPSerial.begin(115200);
  ESPSerial.write("Estou no setup!");

  //flow settings
  device.flow_function = &flow;
  unit1.vector = buffer_char_flow;
  unit2.vector = buffer_char_flow2;
  unit1.used = false; unit2.used = false;
  fluxo.activity = &unit1;
  unit1.next = &unit2;
  //

  //CONNECTIONS
  setup_wifi();

  strcpy(&subsc_topic[4], device_name);

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);

  client.subscribe("dev/");
  client.subscribe(subsc_topic);

  client.publish("dev/CONNECTIONS", device_name);
  /*byte req[80];

    strcpy((char*)req, set_flow);
    device.mqtt_callback("", req, strlen((char*)req) );
    strcpy((char*)req, set_f_luminosity);
    device.mqtt_callback("", req, strlen((char*)req) );*/
}


void loop(){
  if (!client.connected()) {
    reconnect();
  }
  //ESPSerial.write("Cheguei no loop!");
  client.loop();
  fluxo.loop();
}

