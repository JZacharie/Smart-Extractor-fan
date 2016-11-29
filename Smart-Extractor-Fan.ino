#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino
#include <WiFiManager.h>    // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <Ticker.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include "user_config.h"
#include "Smart-Extractor-Fan-Function.h"

void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  int i = 0;
  char message_buff[MQTT_MAX_PACKET_SIZE];
  // handle the MQTT topic of the received message
  
  //Relaie1 VMC 
  if (String(MQTT_SWITCH_COMMAND_TOPIC1).equals(p_topic)) {
    if ((char)p_payload[0] == (char)MQTT_SWITCH_ON_PAYLOAD[0] && (char)p_payload[1] == (char)MQTT_SWITCH_ON_PAYLOAD[1]) {
      if (relayState1 != HIGH) {
        relayState1 = HIGH;
        setRelayState1();
      }
    } else if ((char)p_payload[0] == (char)MQTT_SWITCH_OFF_PAYLOAD[0] && (char)p_payload[1] == (char)MQTT_SWITCH_OFF_PAYLOAD[1] && (char)p_payload[2] == (char)MQTT_SWITCH_OFF_PAYLOAD[2]) {
      if (relayState1 != LOW) {
        relayState1 = LOW;
        setRelayState1();
      }
    }
    mqtt_publish(MQTT_STAT_HUMIDITY,humiditec);
  }

   //Relaie2 PUMP
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
    mqtt_publish(MQTT_STAT_HUMIDITY,humiditec);
  }

  if (String(MQTT_SET_HUMIDITY1).equals(p_topic)) {
    for(i=0; i<p_length; i++) { message_buff[i] = p_payload[i]; }
    message_buff[i] = '\0';
    String msgString = String(message_buff);
    humiditySet1=msgString.toFloat();    
    DEBUG_PRINT(F("Humidity LEVEL 1: "));DEBUG_PRINTLN(humiditySet1);
    saveconfig();
  }
  
  if (String(MQTT_SET_HUMIDITY2).equals(p_topic)) {
    for(i=0; i<p_length; i++) { message_buff[i] = p_payload[i]; }
    message_buff[i] = '\0';
    String msgString = String(message_buff);
    humiditySet2=msgString.toFloat();
    DEBUG_PRINT(F("Humidity LEVEL 2: "));DEBUG_PRINTLN(humiditySet2);
    saveconfig();
  }

  if (String(MQTT_SET_DELAY_PUMP).equals(p_topic)) {
    for(i=0; i<p_length; i++) { message_buff[i] = p_payload[i]; }
    message_buff[i] = '\0';
    String msgString = String(message_buff);
    PUMPbuttontime=msgString.toInt();    
    DEBUG_PRINT(F("PUMPbuttontime: "));DEBUG_PRINTLN(PUMPbuttontime);
    saveconfig();
    mqtt_publish(MQTT_GET_DELAY_PUMP,message_buff);
  }
  
  if (String(MQTT_SET_DELAY_VMC).equals(p_topic)) {
    for(i=0; i<p_length; i++) { message_buff[i] = p_payload[i]; }
    message_buff[i] = '\0';
    String msgString = String(message_buff);
    VMCbuttontime=msgString.toInt();
    DEBUG_PRINT(F("VMCbuttontime: "));DEBUG_PRINTLN(VMCbuttontime);
    saveconfig();
    mqtt_publish(MQTT_GET_DELAY_PUMP,message_buff);
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
        nbread=0;
     }
  }
  else {
    temperature = String(event.temperature);
    temperaturec = &temperature[0];
    float floatreadtemperature=temperature.toFloat();
    nbread++;
  }
  
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
     if (nbread>600){
        sprintf(MQTT_STAT_ERROR, "%06X/stat/error", ESP.getChipId());
        mqtt_publish(MQTT_STAT_ERROR,"Error reading humidity!");
        nbread=0;
     }
  }
  else {
      humidite = String(event.relative_humidity);
      floatreadhumidite=humidite.toFloat();
      humiditec = &humidite[0];
  }

  if (floatreadhumidite<humiditySet1){
     if (relayState1==HIGH){
          relayState1 = LOW;
          setRelayState1();  
          DEBUG_PRINT("INFO: floatreadhumidite < humiditySet1 : WILL STOP THE VMC : ");  DEBUG_PRINT(floatreadhumidite);          DEBUG_PRINT("<");          DEBUG_PRINTLN(humiditySet1);
          mqtt_publish(MQTT_STAT_HUMIDITY,humiditec);
      }
  }

  if (floatreadhumidite>humiditySet2){
     if (relayState1==LOW){
          relayState1 = HIGH;
          setRelayState1();  
          DEBUG_PRINT("INFO: floatreadhumidite>humiditySet2 : WILL START THE VMC : ");  DEBUG_PRINT(floatreadhumidite);         DEBUG_PRINT(">");          DEBUG_PRINTLN(humiditySet2);
          mqtt_publish(MQTT_STAT_HUMIDITY,humiditec);
      }
  }
  
  //Public only 600 Cycle to protect the MQTT server
  if (nbread>600){
      sprintf(MQTT_STAT_TEMP, "%06X/stat/temp", ESP.getChipId());
      mqtt_publish(MQTT_STAT_TEMP,temperaturec);
      sprintf(MQTT_STAT_HUMIDITY, "%06X/stat/humidity", ESP.getChipId());
      mqtt_publish(MQTT_STAT_HUMIDITY,humiditec);
      nbread=0;
  }
}
#endif

void setup() {
Serial.begin(115200);

// init the I/O
pinMode(LED_PIN,    OUTPUT);
pinMode(RELAY_PIN,  OUTPUT);
pinMode(RELAY2_PIN, OUTPUT);

#ifdef NODEMCU
  digitalWrite(RELAY_PIN,  HIGH); //Force OFF not not interract during boot
  digitalWrite(RELAY2_PIN, HIGH);
#endif

pinMode(BUTTON_PIN, INPUT);
pinMode(VMC_PIN,    INPUT_PULLUP);
pinMode(PUMP_PIN,   INPUT_PULLUP);
attachInterrupt(BUTTON_PIN, buttonStateChangedISR, CHANGE);

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
  sprintf(MQTT_CLIENT_ID, "%06X", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT client ID/Hostname: "));  DEBUG_PRINTLN(MQTT_CLIENT_ID);

  sprintf(MQTT_SWITCH_STATE_TOPIC1, "%06X/state/switch1", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT state topic1: "));        DEBUG_PRINTLN(MQTT_SWITCH_STATE_TOPIC1);
  sprintf(MQTT_SWITCH_STATE_TOPIC2, "%06X/state/switch2", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT state topic2: "));        DEBUG_PRINTLN(MQTT_SWITCH_STATE_TOPIC2);

  sprintf(MQTT_SWITCH_COMMAND_TOPIC1, "%06X/switch/switch1", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT command topic1: "));      DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC1);
  sprintf(MQTT_SWITCH_COMMAND_TOPIC2, "%06X/switch/switch2", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT command topic2: "));      DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC2);

  sprintf(MQTT_SET_HUMIDITY1, "%06X/set/humidity1", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT level humidity1: "));     DEBUG_PRINTLN(MQTT_SET_HUMIDITY1);
  sprintf(MQTT_SET_HUMIDITY2, "%06X/set/humidity2", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT level humidity2: "));     DEBUG_PRINTLN(MQTT_SET_HUMIDITY2);

  sprintf(MQTT_SET_DELAY_PUMP, "%06X/set/delay/pump", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT : "));     DEBUG_PRINTLN(MQTT_SET_DELAY_PUMP);
  sprintf(MQTT_GET_DELAY_PUMP, "%06X/get/delay/pump", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT : "));     DEBUG_PRINTLN(MQTT_GET_DELAY_PUMP);

  sprintf(MQTT_SET_DELAY_VMC, "%06X/set/delay/vmc", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT : "));     DEBUG_PRINTLN(MQTT_SET_DELAY_VMC);
  sprintf(MQTT_GET_DELAY_VMC, "%06X/get/delay/vmc", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT : "));     DEBUG_PRINTLN(MQTT_GET_DELAY_VMC);

  // load custom params
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();
  
  String strhumiditySet1 = String(settings.humiditySet1);
  if(isnan(strhumiditySet1.toFloat())){    strhumiditySet1="40.5";   }
  humiditySet1=strhumiditySet1.toFloat();     
  DEBUG_PRINT("INFO: humiditySet1 : "); DEBUG_PRINTLN(humiditySet1);
  
  String strhumiditySet2 = String(settings.humiditySet2);
  if(isnan(strhumiditySet2.toFloat())){    strhumiditySet2="50.5";   }
  humiditySet2=strhumiditySet2.toFloat();   
  DEBUG_PRINT("INFO: humiditySet2 : "); DEBUG_PRINTLN(humiditySet2);

  String strPUMPbuttontime=String(settings.PUMPbuttontime);
  if(isnan(strPUMPbuttontime.toInt())){    strPUMPbuttontime="5000"; }
  PUMPbuttontime=strPUMPbuttontime.toInt();
  DEBUG_PRINT("INFO: PUMPbuttontime : "); DEBUG_PRINTLN(strPUMPbuttontime);
  
  String strVMCbuttontime=String(settings.VMCbuttontime);
  if(isnan(strVMCbuttontime.toInt())){     strVMCbuttontime="5000";  }
  VMCbuttontime=strVMCbuttontime.toInt();
  DEBUG_PRINT("INFO: VMCbuttontime : "); DEBUG_PRINTLN(strVMCbuttontime);

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
  
/*
  String Temp2PUMPbuttontime=String(settings.PUMPbuttontime);
  if(isnan(Temp2PUMPbuttontime.toInt())){
    String strPUMPbuttontime="5000";
    PUMPbuttontime=strPUMPbuttontime.toInt();
  }
  //DEBUG_PRINTLN(strPUMPbuttontime);
  
  String Temp2VMCbuttontime=String(settings.PUMPbuttontime);
  if(isnan(Temp2VMCbuttontime.toInt())){
    String strVMCbuttontime="5000";
    VMCbuttontime=strVMCbuttontime.toInt();
  }
  //DEBUG_PRINTLN(strVMCbuttontime);
*/
  
  if (shouldSaveConfig) {
      saveconfig();
  }

/*    
  String TempVMCbuttontime=String(settings.PUMPbuttontime);
  DEBUG_PRINT(F("INFO: PUMPbuttontime : "));    DEBUG_PRINTLN(settings.PUMPbuttontime);
  DEBUG_PRINT(F("INFO: VMCbuttontime  : "));    DEBUG_PRINTLN(settings.VMCbuttontime);
*/
  
  if (!wifiManager.autoConnect(MQTT_CLIENT_ID)) {
    ESP.reset();
    delay(10);
  }
  
  saveconfig();
  verifyFingerprint();
  mqttClient.setServer(settings.mqttServer, atoi(settings.mqttPort));
  mqttClient.setCallback(callback);

  reconnect();
  ticker.detach();

  //initial Publish
  strhumiditySet1=String(humiditySet1);
  strhumiditySet2=String(humiditySet2);
  strVMCbuttontime=String(VMCbuttontime);
  strPUMPbuttontime=String(PUMPbuttontime);
  
  strhumiditySet1.toCharArray  (charstrhumiditySet1, 50); 
  strhumiditySet2.toCharArray  (charstrhumiditySet2, 50);
  strVMCbuttontime.toCharArray (charVMCbuttontime,   50);
  strPUMPbuttontime.toCharArray(charPUMPbuttontime,  50); 

  mqtt_publish(MQTT_GET_DELAY_PUMP, charPUMPbuttontime);
  mqtt_publish(MQTT_GET_DELAY_VMC,  charVMCbuttontime);
  mqtt_publish(MQTT_GET_HUMIDITY1,  charstrhumiditySet1);
  mqtt_publish(MQTT_GET_HUMIDITY2,  charstrhumiditySet2);

  //initial State
  setRelayState1();
  setRelayState2();
}

void loop() {
  // read the state of the switch into a local variable:
  if (VMCreading != VMClastButtonState) {
    VMClastDebounceTime = millis();
  }
  if (PUMPreading != PUMPlastButtonState) {
    PUMPlastDebounceTime = millis();
  }
  
  if ((millis() - VMClastDebounceTime) > VMCdebounceDelay) {
    if (VMCreading != VMCbuttonState) {
        VMCbuttonState = VMCreading;
        VMCforcedMODE  = true;
        relayState1=LOW; //ON
        setRelayState1();
        DEBUG_PRINT(F("INFO: VMCforcedMODE: "));          DEBUG_PRINTLN(VMCforcedMODE);
        countVMCbuttontime = VMCbuttontime;
    }
  }
  
  if ((millis() - PUMPlastDebounceTime) > PUMPdebounceDelay) {
    if (PUMPreading !=    PUMPbuttonState) {
        PUMPbuttonState = PUMPreading;
        PUMPforcedMODE  = true;
        relayState2=LOW; //ON
        setRelayState2();
        DEBUG_PRINT(F("INFO: PUMPforcedMODE: "));         DEBUG_PRINTLN(PUMPforcedMODE);
        countPUMPbuttontime = PUMPbuttontime;
    }
  }
  
  VMClastButtonState  = VMCreading;
  PUMPlastButtonState = PUMPreading;

  if (countPUMPbuttontime>0){
    countPUMPbuttontime--;
    PUMPforcedMODE=true;
    if (relayState2==LOW){
      relayState2=HIGH;
      setRelayState2();
      DEBUG_PRINT(F("INFO: PUMPforcedMODE: "));         DEBUG_PRINTLN(PUMPforcedMODE);
    }
  }else{
    PUMPforcedMODE=false;
    if (relayState2==HIGH){
      relayState2=LOW;
      setRelayState2();
      DEBUG_PRINT(F("INFO: PUMPforcedMODE: "));         DEBUG_PRINTLN(PUMPforcedMODE);
    }
  }

  if (countVMCbuttontime>0){
    countVMCbuttontime--;
    VMCforcedMODE=true;
    humiditySet2=100;   
  }else{
    VMCforcedMODE=false;
    humiditySet2=savehumiditySet2;
    DEBUG_PRINT(F("INFO: VMCforcedMODE: "));         DEBUG_PRINTLN(VMCforcedMODE);      
  }



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
