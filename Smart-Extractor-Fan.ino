#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino
#include <WiFiManager.h>    // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <Ticker.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>

//NODEMCU or SONOFF
#define NODEMCU
//#define SONOFF

#define DHTSENSOR
#ifdef DHTSENSOR
  #include <DHT.h>
  #include <DHT_U.h>
  #define DHTPIN            2         // Pin which is connected to the DHT sensor.
  #define DHTTYPE DHT22
  //DHT22 Sensor
  uint32_t delayMS;                   //Readtime for DHT22
  int nbread = 0;                     // ToreducethepublicationonMQTT
  DHT_Unified dht(DHTPIN, DHTTYPE);
  String temperature, humidite; //for string add
  char *temperaturec; //Value compatible only with MQTT
  char *humiditec;    //Value compatible only with MQTT
  String humiditySet1 = "62"; // stop  VMC when lower than 62%
  String humiditySet2 = "70"; // Start VMC when upper than 70%
#endif


// TLS support, make sure to edit the fingerprint and the broker address if
// you are not using CloudMQTT
#define           TLS
#ifdef TLS
  const char*       broker      = "iot.svp.zacharie.org"; 
  // SHA1 fingerprint of the certificate
  // openssl x509 -fingerprint -in  <certificate>.crt
  const char*       fingerprint = "DB 0E E4 F4 46 CD EA 92 A3 6C 84 37 DF B6 B9 65 ED 89 3F 2B";
#endif

#define           LED
#ifdef LED
    #include <Adafruit_NeoPixel.h>
    #ifdef __AVR__
      #include <avr/power.h>
    #endif
    // Which pin on the Arduino is connected to the NeoPixels?
    // On a Trinket or Gemma we suggest changing this to 1
    #define PIN            13
    // How many NeoPixels are attached to the Arduino?
    #define NUMPIXELS      4
    // When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
    // Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
    // example for more information on possible values.
    Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
    int delayval = 500; // delay for half a second
#endif

// PIR motion sensor support, make sure to connect a PIR motion sensor to the GPIO14
#define           PIR
#ifdef PIR
  const uint8_t     PIR_SENSOR_PIN = 14;
#endif

#define           DEBUG                       // enable debugging
// macros for debugging
#define           STRUCT_CHAR_ARRAY_SIZE 24   // size of the arrays for MQTT username, password, etc.
#ifdef DEBUG
  #define         DEBUG_PRINT(x)    Serial.print(x)
  #define         DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define         DEBUG_PRINT(x)
  #define         DEBUG_PRINTLN(x)
#endif

// Sonoff properties
const uint8_t     BUTTON_PIN = 0;
const uint8_t     RELAY_PIN  = 12;
const uint8_t     RELAY_PIN2 = 4;
const uint8_t     LED_PIN    = 13;
const uint8_t     LED_PIN2   = 15;

// MQTT
char              MQTT_CLIENT_ID[7]                                  = {0};
char              MQTT_SWITCH_STATE_TOPIC[STRUCT_CHAR_ARRAY_SIZE]    = {0};
char              MQTT_SWITCH_STATE_TOPIC2[STRUCT_CHAR_ARRAY_SIZE]   = {0};
char              MQTT_STAT_TEMP[STRUCT_CHAR_ARRAY_SIZE]             = {0};
char              MQTT_STAT_HUMIDITY[STRUCT_CHAR_ARRAY_SIZE]         = {0};
char              MQTT_STAT_ERROR[STRUCT_CHAR_ARRAY_SIZE]            = {0};
char              MQTT_SWITCH_COMMAND_TOPIC[STRUCT_CHAR_ARRAY_SIZE]  = {0};
char              MQTT_SWITCH_COMMAND_TOPIC2[STRUCT_CHAR_ARRAY_SIZE] = {0};
const char*       MQTT_SWITCH_ON_PAYLOAD                             = "On";
const char*       MQTT_SWITCH_OFF_PAYLOAD                            = "Off";

// Settings for MQTT
typedef struct {
  char            mqttUser[STRUCT_CHAR_ARRAY_SIZE]                  = "";//{0};
  char            mqttPassword[STRUCT_CHAR_ARRAY_SIZE]              = "";//{0};
  char            mqttServer[STRUCT_CHAR_ARRAY_SIZE]                = "";//{0};
  char            mqttPort[6]                                       = "";//{0};
} Settings;

enum CMD {
  CMD_NOT_DEFINED,
  CMD_PIR_STATE_CHANGED,
  CMD_BUTTON_STATE_CHANGED,
};
volatile uint8_t cmd = CMD_NOT_DEFINED;

uint8_t           relayState                                        = HIGH; // HIGH: closed switch
uint8_t           relayState2                                       = HIGH; // HIGH: closed switch
uint8_t           buttonState                                       = HIGH; // HIGH: opened switch
uint8_t           currentButtonState                                = buttonState;
long              buttonStartPressed                                = 0;
long              buttonDurationPressed                             = 0;
uint8_t           pirState                                          = LOW; 
uint8_t           currentPirState                                   = pirState;

Settings          settings;
Ticker            ticker;

#ifdef TLS
  WiFiClientSecure  wifiClient;
#else
  WiFiClient        wifiClient;
#endif

PubSubClient      mqttClient(wifiClient);

///////////////////////////////////////////////////////////////////////////
//   Adafruit IO with SSL/TLS
///////////////////////////////////////////////////////////////////////////
/*
  Function called to verify the fingerprint of the MQTT server certificate
 */
#ifdef TLS
void verifyFingerprint() {
  DEBUG_PRINT(F("INFO: Connecting to "));
  DEBUG_PRINTLN(settings.mqttServer);

  if (!wifiClient.connect(settings.mqttServer, atoi(settings.mqttPort))) {
    DEBUG_PRINTLN(F("ERROR: Connection failed. Halting execution"));
    //reset();
  }

  if (wifiClient.verify(fingerprint, settings.mqttServer)) {
    DEBUG_PRINTLN(F("INFO: Connection secure"));
  } else {
    DEBUG_PRINTLN(F("ERROR: Connection insecure! Halting execution"));
    //reset();
  }
}
#endif
///////////////////////////////////////////////////////////////////////////
//   Sonoff switch
///////////////////////////////////////////////////////////////////////////
/*
  Function called to publish the state of the Sonoff relay
*/
void publishSwitchState() {
  if (relayState == HIGH) {
    if (mqttClient.publish(MQTT_SWITCH_STATE_TOPIC, MQTT_SWITCH_ON_PAYLOAD, true)) {
      DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
      DEBUG_PRINT(MQTT_SWITCH_STATE_TOPIC);
      DEBUG_PRINT(F(". Payload: "));
      DEBUG_PRINTLN(MQTT_SWITCH_ON_PAYLOAD);
    } else {
      DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
    }
  } else {
    if (mqttClient.publish(MQTT_SWITCH_STATE_TOPIC, MQTT_SWITCH_OFF_PAYLOAD, true)) {
      DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
      DEBUG_PRINT(MQTT_SWITCH_STATE_TOPIC);
      DEBUG_PRINT(F(". Payload: "));
      DEBUG_PRINTLN(MQTT_SWITCH_OFF_PAYLOAD);
    } else {
      DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
    }
  }
}

void publishSwitchState2() {
  if (relayState2 == HIGH) {
    if (mqttClient.publish(MQTT_SWITCH_STATE_TOPIC2, MQTT_SWITCH_ON_PAYLOAD, true)) {
      DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
      DEBUG_PRINT(MQTT_SWITCH_STATE_TOPIC2);
      DEBUG_PRINT(F(". Payload: "));
      DEBUG_PRINTLN(MQTT_SWITCH_ON_PAYLOAD);
    } else {
      DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
    }
  } else {
    if (mqttClient.publish(MQTT_SWITCH_STATE_TOPIC2, MQTT_SWITCH_OFF_PAYLOAD, true)) {
      DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
      DEBUG_PRINT(MQTT_SWITCH_STATE_TOPIC2);
      DEBUG_PRINT(F(". Payload: "));
      DEBUG_PRINTLN(MQTT_SWITCH_OFF_PAYLOAD);
    } else {
      DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
    }
  }
}

/*
 Function called to set the state of the relay
 */

#ifdef NODEMCU 
  void setRelayState() {
    digitalWrite(RELAY_PIN, !relayState);
    //digitalWrite(LED_PIN, (relayState + 1) % 2);
    publishSwitchState();
  }
  
  void setRelayState2() {
    digitalWrite(RELAY_PIN2, !relayState2);
    //digitalWrite(LED_PIN2, (relayState + 1) % 2);
    publishSwitchState2();
  }
#else
  void setRelayState() {
    digitalWrite(RELAY_PIN, !relayState);
    digitalWrite(LED_PIN, (relayState + 1) % 2);
    publishSwitchState();
  }
  
  void setRelayState2() {
    digitalWrite(RELAY_PIN2, !relayState2);
    publishSwitchState2();
  }
#endif

///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////
/*
   Function called when a MQTT message arrived
   @param p_topic   The topic of the MQTT message
   @param p_payload The payload of the MQTT message
   @param p_length  The length of the payload
*/
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  // handle the MQTT topic of the received message
  
  //Relaie1
  if (String(MQTT_SWITCH_COMMAND_TOPIC).equals(p_topic)) {
    if ((char)p_payload[0] == (char)MQTT_SWITCH_ON_PAYLOAD[0] && (char)p_payload[1] == (char)MQTT_SWITCH_ON_PAYLOAD[1]) {
      if (relayState != HIGH) {
        relayState = HIGH;
        setRelayState();
      }
    } else if ((char)p_payload[0] == (char)MQTT_SWITCH_OFF_PAYLOAD[0] && (char)p_payload[1] == (char)MQTT_SWITCH_OFF_PAYLOAD[1] && (char)p_payload[2] == (char)MQTT_SWITCH_OFF_PAYLOAD[2]) {
      if (relayState != LOW) {
        relayState = LOW;
        setRelayState();
      }
    }
  }

   //Relaie2
  if (String(MQTT_SWITCH_COMMAND_TOPIC2).equals(p_topic)) {
    if ((char)p_payload[0] == (char)MQTT_SWITCH_ON_PAYLOAD[0] && (char)p_payload[1] == (char)MQTT_SWITCH_ON_PAYLOAD[1]) {
      if (relayState2 != HIGH) {
        relayState2 = HIGH;
        setRelayState2();
      }
    } else if ((char)p_payload[0] == (char)MQTT_SWITCH_OFF_PAYLOAD[0] && (char)p_payload[1] == (char)MQTT_SWITCH_OFF_PAYLOAD[1] && (char)p_payload[2] == (char)MQTT_SWITCH_OFF_PAYLOAD[2]) {
      if (relayState2 != LOW) {
        relayState2 = LOW;
        setRelayState2();
      }
    }
  }
}


/*
  Function called to connect/reconnect to the MQTT broker
 */
void reconnect() {
  uint8_t i = 0;
  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, settings.mqttUser, settings.mqttPassword)) {
      DEBUG_PRINTLN(F("INFO: The client is successfully connected to the MQTT broker"));
    } else {
      DEBUG_PRINTLN(F("ERROR: The connection to the MQTT broker failed"));
      DEBUG_PRINT(F("Username: "));
      DEBUG_PRINTLN(settings.mqttUser);
      DEBUG_PRINT(F("Password: "));
      DEBUG_PRINTLN(settings.mqttPassword);
      DEBUG_PRINT(F("Broker: "));
      DEBUG_PRINTLN(settings.mqttServer);
      delay(1000);
      if (i == 3) {
        //reset();
      }
      i++;
    }
  }

  if (mqttClient.subscribe(MQTT_SWITCH_COMMAND_TOPIC)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));
    DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed. Topic: "));
    DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC);
  }
  
  if (mqttClient.subscribe(MQTT_SWITCH_COMMAND_TOPIC2)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));
    DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC2);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed. Topic: "));
    DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC2);
  }
}

///////////////////////////////////////////////////////////////////////////
//   WiFiManager
///////////////////////////////////////////////////////////////////////////
/*
  Function called to toggle the state of the LED
 */
void tick() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// flag for saving data
bool shouldSaveConfig = false;

// callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  ticker.attach(0.2, tick);
}


/*
  Function called to restart the switch
 */
void restart() {
  DEBUG_PRINTLN(F("INFO: Restart..."));
    ESP.reset();
  delay(1000);
}

/*
  Function called to reset the configuration of the switch
 */
void reset() {
  DEBUG_PRINTLN(F("INFO: Reset..."));
  //WiFi.disconnect();
  delay(1000);
  ESP.reset();
  delay(1000);
}

///////////////////////////////////////////////////////////////////////////
//   ISR
///////////////////////////////////////////////////////////////////////////
/*
  Function called when the button is pressed/released
 */
void buttonStateChangedISR() {
  cmd = CMD_BUTTON_STATE_CHANGED;
}

/*
  Function called when the PIR sensor detects the biginning/end of a mouvement
 */
 #ifdef PIR
void pirStateChangedISR() {
  cmd = CMD_PIR_STATE_CHANGED;
}
#endif

#ifdef DHTSENSOR
void dht22(){
  delay(delayMS);
 // Get temperature event and print its value.
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  
  //DEBUG_PRINT(nbread);
  
  if (isnan(event.temperature)) {
     if (nbread>600){
        sprintf(MQTT_STAT_ERROR, "%06X/stat/temp", ESP.getChipId());
        if (mqttClient.publish(MQTT_STAT_ERROR, "Error reading temperature!", true)) {
          DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
          DEBUG_PRINT(MQTT_STAT_ERROR);
          DEBUG_PRINT(F(". Payload: "));
          DEBUG_PRINTLN("Error reading temperature!");
        } else {
          DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
        }
        nbread=0;
     }
  }
  else {
    temperature = String(event.temperature);
    temperaturec = &temperature[0];
    nbread++;
  }
  
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
     if (nbread>600){
        sprintf(MQTT_STAT_ERROR, "%06X/stat/temp", ESP.getChipId());
        if (mqttClient.publish(MQTT_STAT_ERROR, "Error reading humidity!", true)) {
          DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
          DEBUG_PRINT(MQTT_STAT_ERROR);
          DEBUG_PRINT(F(". Payload: "));
          DEBUG_PRINTLN("Error reading humidity!");
        } else {
          DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
        }
        nbread=0;
     }
  }
  else {
      humidite = String(event.relative_humidity);
      humiditec = &humidite[0];
  }
    if (nbread>600){
      sprintf(MQTT_STAT_TEMP, "%06X/stat/temp", ESP.getChipId());
      if (mqttClient.publish(MQTT_STAT_TEMP, temperaturec, true)) {
        DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
        DEBUG_PRINT(MQTT_STAT_TEMP);
        DEBUG_PRINT(F(". Payload: "));
        DEBUG_PRINTLN(temperaturec);
        #ifdef LED
          for(int i=0;i<NUMPIXELS;i++){
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            pixels.setPixelColor(i, pixels.Color(0,150,0)); // Moderately bright green color.
            pixels.show(); // This sends the updated pixel color to the hardware.
          }
        #endif
      } else {
        DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
        #ifdef LED
          for(int i=0;i<NUMPIXELS;i++){
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            pixels.setPixelColor(i, pixels.Color(150,0,0)); // Moderately bright green color.
            pixels.show(); // This sends the updated pixel color to the hardware.
          }
        #endif
      }
      sprintf(MQTT_STAT_HUMIDITY, "%06X/stat/humidity", ESP.getChipId());
      if (mqttClient.publish(MQTT_STAT_HUMIDITY, humiditec, true)) {
        DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
        DEBUG_PRINT(MQTT_STAT_HUMIDITY);
        DEBUG_PRINT(F(". Payload: "));
        DEBUG_PRINTLN(humiditec);
        #ifdef LED
          for(int i=0;i<NUMPIXELS;i++){
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            pixels.setPixelColor(i, pixels.Color(0,150,0)); // Moderately bright green color.
            pixels.show(); // This sends the updated pixel color to the hardware.
          }
        #endif
      } else {
        DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
        #ifdef LED
          for(int i=0;i<NUMPIXELS;i++){
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            pixels.setPixelColor(i, pixels.Color(150,0,0)); // Moderately bright green color.
            pixels.show(); // This sends the updated pixel color to the hardware.
          }
        #endif
      }
       nbread=0;
    }
/*     
     if (humidite>humiditySet2){
          Serial.println("power1: "+powerset1);
          if (VMCForcedmode = "Off"){
            //digitalWrite(Relay_Pin_1, LOW);
            mqtt.publish("/stat/ESP_10AC41/power1", "On");
          }else{
            Serial.println("Forcedmode: "+String(counttime));
            mqtt.publish("/stat/ESP_10AC41/forcedmod", (char*)counttime);
          }
     }
     if (humidite<humiditySet1){
          Serial.println("power1: "+ String(counttime));
          if (VMCForcedmode = "Off"){
            //digitalWrite(Relay_Pin_1, HIGH);
            mqtt.publish("/stat/ESP_10AC41/power1", "Off");
          }else{
            Serial.println("Forcedmode: "+ String(counttime));
            mqtt.publish("/stat/ESP_10AC41/forcedmod", (char*)counttime);
          }
     }
*/
}
#endif

///////////////////////////////////////////////////////////////////////////
//   Setup() and loop()
///////////////////////////////////////////////////////////////////////////
void setup() {

       
#ifdef LED
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
  pixels.begin(); // This initializes the NeoPixel library.
#endif

        #ifdef LED
          for(int i=0;i<NUMPIXELS;i++){
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            pixels.setPixelColor(i, pixels.Color(150,0,0)); // Moderately bright green color.
            pixels.show(); // This sends the updated pixel color to the hardware.
          }
        #endif  
  Serial.begin(115200);

#ifdef DHTSENSOR
  // Initialize device.
  dht.begin();
  Serial.println("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 10000;
#endif

  // init the I/O
  pinMode(LED_PIN,    OUTPUT);
  pinMode(RELAY_PIN,  OUTPUT);
  pinMode(RELAY_PIN2, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(BUTTON_PIN, buttonStateChangedISR, CHANGE);

#ifdef NODEMCU
    //switch off during begining
    relayState = LOW;
    relayState2 = LOW;
    setRelayState();
    setRelayState2();
#endif
  
#ifdef PIR
  pinMode(PIR_SENSOR_PIN, INPUT);
  attachInterrupt(PIR_SENSOR_PIN, pirStateChangedISR, CHANGE);
#endif
  ticker.attach(0.6, tick);

  // get the Chip ID of the switch and use it as the MQTT client ID
  sprintf(MQTT_CLIENT_ID, "%06X", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT client ID/Hostname: "));
  DEBUG_PRINTLN(MQTT_CLIENT_ID);

  // set the state topic: <Chip ID>/state/switch
  sprintf(MQTT_SWITCH_STATE_TOPIC, "%06X/state/switch1", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT state topic: "));
  DEBUG_PRINTLN(MQTT_SWITCH_STATE_TOPIC);

  // set the command topic: <Chip ID>/switch/switch1
  sprintf(MQTT_SWITCH_COMMAND_TOPIC, "%06X/switch/switch1", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT command topic: "));
  DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC);

    // set the state topic: <Chip ID>/state/switch2
  sprintf(MQTT_SWITCH_STATE_TOPIC2, "%06X/state/switch2", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT state topic2: "));
  DEBUG_PRINTLN(MQTT_SWITCH_STATE_TOPIC2);

  // set the command topic: <Chip ID>/switch/switch2
  sprintf(MQTT_SWITCH_COMMAND_TOPIC2, "%06X/switch/switch2", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT command topic2: "));
  DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC2);

  // load custom params
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();

#ifdef TLS
  WiFiManagerParameter custom_text("<p>MQTT username, password and broker port</p>");
  WiFiManagerParameter custom_mqtt_server("mqtt-server", "MQTT Broker IP", "m21.cloudmqtt.com", STRUCT_CHAR_ARRAY_SIZE, "disabled");
#else
  WiFiManagerParameter custom_text("<p>MQTT username, password, broker IP address and broker port</p>");
  WiFiManagerParameter custom_mqtt_server("mqtt-server", "MQTT Broker IP", settings.mqttServer, STRUCT_CHAR_ARRAY_SIZE);
#endif
  WiFiManagerParameter custom_mqtt_user("mqtt-user", "MQTT User", settings.mqttUser, STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_mqtt_password("mqtt-password", "MQTT Password", settings.mqttPassword, STRUCT_CHAR_ARRAY_SIZE, "type = \"password\"");
  WiFiManagerParameter custom_mqtt_port("mqtt-port", "MQTT Broker Port", settings.mqttPort, 6);

  WiFiManager wifiManager;

  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180);
  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (!wifiManager.autoConnect(MQTT_CLIENT_ID)) {
    ESP.reset();
    delay(1000);
  }

  if (shouldSaveConfig) {
#ifdef TLS
    strcpy(settings.mqttServer,   broker);
#else
    strcpy(settings.mqttServer,   custom_mqtt_server.getValue());
#endif
    strcpy(settings.mqttUser,     custom_mqtt_user.getValue());
    strcpy(settings.mqttPassword, custom_mqtt_password.getValue());
    strcpy(settings.mqttPort,     custom_mqtt_port.getValue());

    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
  }

#ifdef TLS
    verifyFingerprint();
#endif

  // configure MQTT
  mqttClient.setServer(settings.mqttServer, atoi(settings.mqttPort));
  mqttClient.setCallback(callback);

  // connect to the MQTT broker
  reconnect();
  
  ticker.detach();

  setRelayState();
  setRelayState2();
}


void loop() {

switch (cmd) {
    case CMD_NOT_DEFINED:
      // do nothing
      break;

#ifdef PIR
    case CMD_PIR_STATE_CHANGED:
      currentPirState = digitalRead(PIR_SENSOR_PIN);
      if (pirState != currentPirState) {
        if (pirState == LOW && currentPirState == HIGH) {
          if (relayState != HIGH) {
            relayState = HIGH; // closed
            setRelayState();
          }
        } else if (pirState == HIGH && currentPirState == LOW) {
          if (relayState != LOW) {
            relayState = LOW; // opened
            setRelayState();
          }
        }
        pirState = currentPirState;
      }
      cmd = CMD_NOT_DEFINED;
      break;
#endif

    case CMD_BUTTON_STATE_CHANGED:
      currentButtonState = digitalRead(BUTTON_PIN);
      if (buttonState != currentButtonState) {
        // tests if the button is released or pressed
        if (buttonState == LOW && currentButtonState == HIGH) {
          buttonDurationPressed = millis() - buttonStartPressed;
          if (buttonDurationPressed < 500) {
            relayState = relayState == HIGH ? LOW : HIGH;
            setRelayState();
          } else if (buttonDurationPressed < 3000) {
            restart();
          } else {
            reset();
          }
        } else if (buttonState == HIGH && currentButtonState == LOW) {
          buttonStartPressed = millis();
        }
        buttonState = currentButtonState;
      }
      cmd = CMD_NOT_DEFINED;
      break;
  }

  yield();
  
  // keep the MQTT client connected to the broker
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  // Delay between measurements.

  #ifdef DHTSENSOR
    dht22();
  #endif
 
  yield();
}
