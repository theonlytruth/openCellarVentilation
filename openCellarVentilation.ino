/***************************************************
  MQTT-CellarVentilation by Carsten Pietzsch

  based on Adafruit MQTT Library ESP8266 Example
    https://github.com/adafruit/Adafruit_MQTT_Library/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino

Original Header Text:

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
 

/****************************** Includes **************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHTesp.h"


/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "MyWiFi"
#define WLAN_PASS       "MyPassword"


/************************* MQTT Setup *********************************/

#define MQTT_SERVER      "broker.hivemq.com"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME    ""
#define MQTT_KEY         ""


/*************************** Pin Definition ************************************/
#define DHT_OUT_PIN     16
#define DHT_IN_PIN      2
#define FAN_PIN         12


/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

DHTesp dht_o;
DHTesp dht_i;

/****************************** Feeds ***************************************/

// Setup some feeds for publishing.
Adafruit_MQTT_Publish pub_t_o    = Adafruit_MQTT_Publish(&mqtt, "myPrefix/ventilation/t_o");
Adafruit_MQTT_Publish pub_t_i    = Adafruit_MQTT_Publish(&mqtt, "myPrefix/ventilation/t_i", 0);
Adafruit_MQTT_Publish pub_h_o    = Adafruit_MQTT_Publish(&mqtt, "myPrefix/ventilation/h_o", 0);
Adafruit_MQTT_Publish pub_h_i    = Adafruit_MQTT_Publish(&mqtt, "myPrefix/ventilation/h_i", 0);
Adafruit_MQTT_Publish pub_dp_o   = Adafruit_MQTT_Publish(&mqtt, "myPrefix/ventilation/dp_o", 0);
Adafruit_MQTT_Publish pub_dp_i   = Adafruit_MQTT_Publish(&mqtt, "myPrefix/ventilation/dp_i", 0);
Adafruit_MQTT_Publish pub_switch = Adafruit_MQTT_Publish(&mqtt, "myPrefix/ventilation/switch", 0);

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe sub_switch   = Adafruit_MQTT_Subscribe(&mqtt, "myPrefix/ventilation/switch");
Adafruit_MQTT_Subscribe sub_automode = Adafruit_MQTT_Subscribe(&mqtt, "myPrefix/ventilation/automode");


/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println(F("MQTT-CellarVentilation"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscriptions
    mqtt.subscribe(&sub_switch);
    mqtt.subscribe(&sub_automode);
    
  
  dht_o.setup(DHT_OUT_PIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 
  dht_i.setup(DHT_IN_PIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN,LOW);
}


void loop(void){

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  
  //DHT specific part
  delay(dht_o.getMinimumSamplingPeriod());
  delay(dht_i.getMinimumSamplingPeriod());

  float h_o     = dht_o.getHumidity();
  float t_o     = dht_o.getTemperature();
  float dp_o    = dht_o.computeDewPoint(t_o,h_o,0);

  float h_i     = dht_i.getHumidity();
  float t_i     = dht_i.getTemperature();
  float dp_i    = dht_i.computeDewPoint(t_i,h_i,0);

  bool _switch;

  Serial.print("Status innen: ");
  Serial.print(dht_i.getStatusString());
  Serial.print("\t");
  Serial.print("Feuchte: "); 
  Serial.print(h_i, 1);
  Serial.print("\t\t");
  Serial.print("Temperatur: "); 
  Serial.print(t_i, 1);
  Serial.print("\t\t");
  Serial.print("Taupunkt: "); 
  Serial.print(dp_i, 1);

  
  Serial.println("");

  Serial.print("Status außen: ");
  Serial.print(dht_o.getStatusString());
  Serial.print("\t");
  Serial.print("Feuchte: "); 
  Serial.print(h_o, 1);
  Serial.print("\t\t");
  Serial.print("Temperatur: "); 
  Serial.print(t_o, 1);
  Serial.print("\t\t");
  Serial.print("Taupunkt: "); 
  Serial.print(dp_o, 1);

  if (dp_o < dp_i) _switch = 1; else _switch = 0;
  digitalWrite(FAN_PIN,!_switch);


  Serial.println("");


    if (! pub_t_o.publish(t_o)) {
    Serial.println(F("t_o Failed"));
  } else {
    Serial.println(F("t_o OK!"));
  }

    if (! pub_t_i.publish(t_i)) {
    Serial.println(F("t_i Failed"));
  } else {
    Serial.println(F("t_i OK!"));
  }

    if (! pub_h_o.publish(h_o)) {
    Serial.println(F("h_o Failed"));
  } else {
    Serial.println(F("h_o OK!"));
  }

    if (! pub_h_i.publish(h_i)) {
    Serial.println(F("h_i Failed"));
  } else {
    Serial.println(F("h_i OK!"));
  }

    if (! pub_dp_o.publish(dp_o)) {
    Serial.println(F("dp_o Failed"));
  } else {
    Serial.println(F("dp_o OK!"));
  }

    if (! pub_dp_i.publish(dp_i)) {
    Serial.println(F("dp_i Failed"));
  } else {
    Serial.println(F("dp_i OK!"));
  }

    if (! pub_switch.publish(_switch)) {
    Serial.println(F("switch Failed"));
  } else {
    Serial.println(F("switch OK!"));
  }

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 2 seconds...");
       mqtt.disconnect();
       delay(2000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
