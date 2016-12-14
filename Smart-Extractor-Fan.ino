#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino
#include <WiFiManager.h>    // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <ArduinoJson.h>    // https://github.com/bblanchon/ArduinoJson
#include <Ticker.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include "user_config.h"
#include "Smart-Extractor-Fan-Function.h"

void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  int i = 0;
  unsigned long IRcode;
  char message_buff[MQTT_MAX_PACKET_SIZE];

  // if recieved a correct topic message.
  if (String(MQTT_SET_TOPIC).equals(p_topic)) {
    for(i=0; i<p_length; i++) { message_buff[i] = p_payload[i]; }
    message_buff[i] = '\0';

    //Convert Payload to JSON Object
    StaticJsonBuffer<1000> jsonBuffer; 
    JsonObject& root = jsonBuffer.parseObject(message_buff);
        
    if (!root.success())  {
      Serial.println("parseObject() failed");
      DEBUG_PRINTLN(message_buff);  
    }else{
       HUM_L1           = root["HUM_L1"].as<float>();
       HUM_L2           = root["HUM_L2"].as<float>();
       VMC_B_TIME       = root["VMC_B_TIME"].as<int>();
       VMC_B_STAT       = root["VMC_B_STAT"].as<bool>();
       PUMP_B_TIME      = root["PUMP_B_TIME"].as<int>();
       PUMP_B_STAT      = root["PUMP_B_STAT"].as<bool>();
       saveconfig();
    }
   DEBUG_PRINT("INFO : HUM_L1 : ")     ; DEBUG_PRINTLN(HUM_L1);
   DEBUG_PRINT("INFO : HUM_L2 : ")     ; DEBUG_PRINTLN(HUM_L2);
   DEBUG_PRINT("INFO : VMC_B_TIME : ") ; DEBUG_PRINTLN(VMC_B_TIME);
   DEBUG_PRINT("INFO : VMC_B_STAT : ") ; DEBUG_PRINTLN(VMC_B_STAT);
   DEBUG_PRINT("INFO : PUMP_B_TIME : "); DEBUG_PRINTLN(PUMP_B_TIME);
   DEBUG_PRINT("INFO : PUMP_B_STAT : "); DEBUG_PRINTLN(PUMP_B_STAT);
  }
}

#ifdef DHTSENSOR
void dht22(){
  delay(delayMS);
 // Get temperature event and print its value.
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  
  if (isnan(event.temperature)) {
     if (nbread>600){
        sprintf(MQTT_STAT_ERROR, "%06X/stat/error", ESP.getChipId());
        mqtt_publish(MQTT_STAT_ERROR,"Error reading temperature!");
        DEBUG_PRINTLN("Error reading temperature!");
        nbread=0;
     }
  }
  else {
    String temperature = String(event.temperature);
    //String temperaturec = &temperature[0];
    TEMP_R=temperature.toFloat();
  }
  
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
     if (nbread>600){
        sprintf(MQTT_STAT_ERROR, "%06X/stat/error", ESP.getChipId());
        mqtt_publish(MQTT_STAT_ERROR,"Error reading humidity!");
        DEBUG_PRINTLN("Error reading humidity!");
        nbread=0;
     }
  }
  else {
      String humidite = String(event.relative_humidity);
      HUM_R=humidite.toFloat();
      // humiditec = &humidite[0];
      nbread++;
  }

  pump();
  vmc();
 
  //Public only NbCycleBeforSendMQTT Cycle to protect the MQTT server
  if (nbread>NbCycleBeforSendMQTT){
      setRelayStateandpublish();
      nbread=0;
  }
}
#endif

void setup() {
Serial.begin(115200);

// init the I/O
pinMode(LED_PIN,    OUTPUT);
pinMode(RELAY1_PIN, OUTPUT);
pinMode(RELAY2_PIN, OUTPUT);
pinMode(BUTTON_PIN, INPUT);
pinMode(VMC_PIN,    INPUT_PULLUP);
pinMode(PUMP_PIN,   INPUT_PULLUP);
attachInterrupt(BUTTON_PIN, buttonStateChangedISR, CHANGE);

#ifdef NODEMCU
    //switch off during begining
    PUMP_C_STAT = HIGH;
    VMC_C_STAT  = HIGH;
    digitalWrite(RELAY1_PIN, PUMP_C_STAT);
    digitalWrite(RELAY2_PIN, VMC_C_STAT);
#endif

#ifdef LED
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
  pixels.begin(); // This initializes the NeoPixel library.
  setled(0,0,150); //Blue
#endif  

#ifdef DHTSENSOR
  initDHTSENSOR();
#endif
  
#ifdef PIR
  pinMode(PIR_SENSOR_PIN, INPUT);
  attachInterrupt(PIR_SENSOR_PIN, pirStateChangedISR, CHANGE);
#endif
  ticker.attach(0.6, tick);
  

  // load custom params
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();
  
  String String_HUM_L1 = String(settings.HUM_L1);
  if(isnan(String_HUM_L1.toFloat())){    String_HUM_L1="40.5";   } //TODO default value
  HUM_L1=String_HUM_L1.toFloat();     
  
  String String_HUM_L2 = String(settings.HUM_L2);
  if(isnan(String_HUM_L2.toFloat())){    String_HUM_L2="50.5";   }
  HUM_L2=String_HUM_L2.toFloat();   

  String strPUMPbuttontime=String(settings.PUMP_B_TIME);
  if(isnan(strPUMPbuttontime.toInt())){    strPUMPbuttontime="50"; }
  PUMP_B_TIME=strPUMPbuttontime.toInt();
  
  String strVMCbuttontime=String(settings.VMC_B_TIME);
  if(isnan(strVMCbuttontime.toInt())){     strVMCbuttontime="50";  }
  VMC_B_TIME=strVMCbuttontime.toInt();

  strmqtt_server   = String (settings.mqttServer);
  strmqtt_user     = String (settings.mqttUser);
  strmqtt_password = String (settings.mqttPassword);
  strmqtt_port     = String (settings.mqttPort);

  strmqtt_server.toCharArray   (mqtt_server,   50);
  strmqtt_user.toCharArray     (mqtt_user,     50);
  strmqtt_password.toCharArray (mqtt_password, 50);
  strmqtt_port.toCharArray     (mqtt_port,     50);
  
  WiFiManagerParameter custom_text         ("<p>MQTT username, password and broker port</p>");
  WiFiManagerParameter custom_mqtt_server  ("mqtt-server"  , "MQTT Broker IP"  , mqtt_server,         STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_mqtt_user    ("mqtt-user"    , "MQTT User"       , mqtt_user,           STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_mqtt_password("mqtt-password", "MQTT Password"   , mqtt_password,       STRUCT_CHAR_ARRAY_SIZE, "type = \"password\"");
  WiFiManagerParameter custom_mqtt_port    ("mqtt-port"    , "MQTT Broker Port", mqtt_port, 6);
  
  WiFiManager wifiManager;

  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180); //180
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  strcpy(settings.mqttServer,   custom_mqtt_server.getValue());
  strcpy(settings.mqttUser,     custom_mqtt_user.getValue());
  strcpy(settings.mqttPassword, custom_mqtt_password.getValue());
  strcpy(settings.mqttPort,     custom_mqtt_port.getValue());

  strPUMPbuttontime=String(settings.PUMP_B_TIME);
  if(isnan(strPUMPbuttontime.toInt())){
    PUMP_B_TIME=50;
  }
  
  strVMCbuttontime=String(settings.VMC_B_TIME);
  if(isnan(strVMCbuttontime.toInt())){
    VMC_B_TIME=50;
  }

  if (shouldSaveConfig) {
      saveconfig();
  }
    
  //String TempVMCbuttontime=String(settings.VMC_B_TIME);
  DEBUG_PRINT(F("INFO: PUMP_B_TIME : "));    DEBUG_PRINTLN(settings.PUMP_B_TIME);
  DEBUG_PRINT(F("INFO: VMC_B_TIME  : "));    DEBUG_PRINTLN(settings.VMC_B_TIME);
  
  if (!wifiManager.autoConnect(MQTT_CLIENT_ID)) {
    ESP.reset();
    delay(10);
  }

  verifyFingerprint();
  prep_MQTT_topic();
  mqttClient.setServer(settings.mqttServer, atoi(settings.mqttPort));
  mqttClient.setCallback(callback);
  reconnect();
  delay(100);
  setled(0,0,120);
  ticker.detach();
}

void loop() {

  // read the state of the switch into a local variable:
  readbutton();
  

switch (cmd) {
    case CMD_NOT_DEFINED:
      // do nothing
      break;

#ifdef PIR
    case CMD_PIR_STATE_CHANGED:
      currentPirState = digitalRead(PIR_SENSOR_PIN);
      if (pirState != currentPirState) {
        if (pirState == LOW && currentPirState == HIGH) {
          if (relayState1 != HIGH) {
            relayState1 = HIGH; // closed
            setRelayState1();
          }
        } else if (pirState == HIGH && currentPirState == LOW) {
          if (relayState1 != LOW) {
            relayState1 = LOW; // opened
            setRelayState1();
          }
        }
        pirState = currentPirState;
      }
      cmd = CMD_NOT_DEFINED;
      break;
#endif

#ifdef SONOFF
    case CMD_BUTTON_STATE_CHANGED:
      currentButtonState = digitalRead(BUTTON_PIN);
      if (buttonState != currentButtonState) {
        // tests if the button is released or pressed
        if (buttonState == LOW && currentButtonState == HIGH) {
          buttonDurationPressed = millis() - buttonStartPressed;
          if (buttonDurationPressed < 500) {
            relayState1 = relayState1 == HIGH ? LOW : HIGH;
            setRelayState1();
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
#endif
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
