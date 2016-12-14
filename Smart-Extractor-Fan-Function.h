#define DHTSENSOR
#ifdef DHTSENSOR
  #include <DHT.h>
  #include <DHT_U.h>
  #define DHTPIN            2         // Pin which is connected to the DHT sensor.
  #define DHTTYPE DHT22
  //DHT22 Sensor
  uint32_t delayMS;                   //Readtime for DHT22
  int nbread = 600;                   // ToreducethepublicationonMQTT
  DHT_Unified dht(DHTPIN, DHTTYPE);
  float HUM_L1; // stop  VMC when lower than xx%
  float HUM_L2; // Start VMC when upper than yy%
  float TEMP_R;
  float HUM_R;
#endif

int  VMC_B_COUNT  = 0;
bool VMC_B_STAT   = 0; 
int  VMC_B_TIME   = 0;
bool VMC_C_STAT   = 0;
bool VMCbuttonState;
//int countVMCbuttontime=0;

int  PUMP_B_COUNT = 0;
bool PUMP_B_STAT  = 0;
int  PUMP_B_TIME  = 0;
bool PUMP_C_STAT  = 0; 
bool PUMPbuttonState;
//int countPUMPbuttontime=0;
  
// flag for saving data
bool shouldSaveConfig = false;
//int NbCycleBeforSendMQTT= 600;

#define           LED
#ifdef LED
    #include <Adafruit_NeoPixel.h>
    #ifdef __AVR__
      #include <avr/power.h>
    #endif
    Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, STRIP_PIN, NEO_GRB + NEO_KHZ800);
    int delayval = 500; // delay for half a second
#endif

#define           STRUCT_CHAR_ARRAY_SIZE 24   // size of the arrays for MQTT username, password, etc.

#define           DEBUG                       // enable debugging
#ifdef DEBUG
  #define         DEBUG_PRINT(x)    Serial.print(x)
  #define         DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define         DEBUG_PRINT(x)
  #define         DEBUG_PRINTLN(x)
#endif


//#define           PIR // PIR motion sensor support, make sure to connect a PIR motion sensor to the GPIO14
#ifdef PIR
  const uint8_t     PIR_SENSOR_PIN = 14;
#endif

// MQTT
char              MQTT_CLIENT_ID[7]                                  = {0};
char              MQTT_GET_TOPIC[STRUCT_CHAR_ARRAY_SIZE]             = {0};
char              MQTT_SET_TOPIC[STRUCT_CHAR_ARRAY_SIZE]             = {0};
char              MQTT_STAT_ERROR[STRUCT_CHAR_ARRAY_SIZE]            = {0};

// Setting for WIFI
char              mqtt_server[STRUCT_CHAR_ARRAY_SIZE]                = {0};
char              mqtt_user[STRUCT_CHAR_ARRAY_SIZE]                  = {0};
char              mqtt_password[STRUCT_CHAR_ARRAY_SIZE]              = {0};
char              mqtt_port[STRUCT_CHAR_ARRAY_SIZE]                  = {0};
char              mqttHUM_L1[STRUCT_CHAR_ARRAY_SIZE]                 = {0};
char              mqttHUM_L2[STRUCT_CHAR_ARRAY_SIZE]                 = {0};
char              mqttPUMP_B_TIME[STRUCT_CHAR_ARRAY_SIZE]            = {0};
char              mqttVMC_B_TIME[STRUCT_CHAR_ARRAY_SIZE]             = {0};

char*             MQTT_SWITCH_ON_PAYLOAD                             = "On";
char*             MQTT_SWITCH_OFF_PAYLOAD                            = "Off";

// Settings for VMC
typedef struct {
    char            mqttUser[STRUCT_CHAR_ARRAY_SIZE]                 = "";
    char            mqttPassword[STRUCT_CHAR_ARRAY_SIZE]             = "";
    char            mqttServer[STRUCT_CHAR_ARRAY_SIZE]               = "";
    char            mqttPort[6]                                      = "";
    float           HUM_L1                                           = 49;
    float           HUM_L2                                           = 51;
    int             PUMP_B_TIME                                      = 5000;
    int             VMC_B_TIME                                       = 5000;
} Settings;

enum CMD {
  CMD_NOT_DEFINED,
  CMD_PIR_STATE_CHANGED,
  CMD_BUTTON_STATE_CHANGED,
};

volatile uint8_t cmd = CMD_NOT_DEFINED;

// To debounce the Button
int               VMClastButtonState                                = LOW;      // the previous reading from the input pin
long              VMClastDebounceTime                               = 0;        // the last time the output pin was toggled
long              VMCdebounceDelay                                  = 2;        // the debounce time; increase if the output flickers

int               PUMPlastButtonState                               = LOW;      // the previous reading from the input pin
long              PUMPlastDebounceTime                              = 0;        // the last time the output pin was toggled
long              PUMPdebounceDelay                                 = 2;        // the debounce time; increase if the output flickers

Settings          settings;
Ticker            ticker;
WiFiClientSecure  wifiClient;
PubSubClient      mqttClient(wifiClient);

#ifdef LED
void setled(int a, int b, int c){
          for(int i=0;i<NUMPIXELS;i++){
            pixels.setPixelColor(i, pixels.Color(a,b,c)); 
            pixels.show(); // This sends the updated pixel color to the hardware.
          }
      
}
#endif

void restart() { /*    Function called to restart the switch  */
  DEBUG_PRINTLN(F("INFO: Restart..."));
    ESP.reset();
  delay(100);
}

void Subscribe_MQTT(){
  if (mqttClient.subscribe(MQTT_SET_TOPIC)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));  DEBUG_PRINTLN(MQTT_GET_TOPIC);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed. Topic: "));    DEBUG_PRINTLN(MQTT_GET_TOPIC);
  }
}

void reset() {             /*    Function called to reset the configuration of the switch  */
  DEBUG_PRINTLN(F("INFO: Reset..."));
//  WiFi.disconnect();
//  ESP.reset();
//  delay(1000);
}

void reconnect() {         /*  Function called to connect/reconnect to the MQTT broker  */
  uint8_t i = 0;
  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, settings.mqttUser, settings.mqttPassword)) {
      DEBUG_PRINTLN(F("INFO: The client is successfully connected to the MQTT broker"));
        delay(100);
        Subscribe_MQTT();
    } else {
      DEBUG_PRINTLN(F("ERROR: The connection to the MQTT broker failed"));
      DEBUG_PRINT(F("Username: "));      DEBUG_PRINTLN(settings.mqttUser);
      DEBUG_PRINT(F("Broker: "));        DEBUG_PRINTLN(settings.mqttServer);
      delay(100);
      if (i == 3) {
        reset();
      }
      i++;
    }
  }
}

void initDHTSENSOR(){     // Initialize device.

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
}

void mqtt_publish(char MQTT_BUFFER_TOPIC[STRUCT_CHAR_ARRAY_SIZE]  , char*    MQTT_BUFFER_PAYLOAD  ) {
    if (mqttClient.publish(MQTT_BUFFER_TOPIC, MQTT_BUFFER_PAYLOAD , true)) {
      DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));      DEBUG_PRINT(MQTT_BUFFER_TOPIC);      DEBUG_PRINT(F(" Payload: ")); DEBUG_PRINTLN(MQTT_BUFFER_PAYLOAD);
      #ifdef LED
          setled(0,150,0); //Green
      #endif
    } else {
      DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
      #ifdef LED
          setled(150,0,0); //Red
      #endif
    }
}

void verifyFingerprint() { /*  Function called to verify the fingerprint of the MQTT server certificate */
  DEBUG_PRINT(F("INFO: Connecting to "));
  DEBUG_PRINTLN(settings.mqttServer);

  if (!wifiClient.connect(settings.mqttServer, atoi(settings.mqttPort))) {
    DEBUG_PRINTLN(F("ERROR: Connection failed. Halting execution"));
    reset();
  }

  if (wifiClient.verify(fingerprint, settings.mqttServer)) {
    DEBUG_PRINTLN(F("INFO: Connection secure"));
  } else {
    DEBUG_PRINTLN(F("ERROR: Connection insecure! Halting execution"));
    reset();
  }
}

void setRelayStateandpublish() {
   
   //Affect les variables
   digitalWrite(RELAY1_PIN, PUMP_C_STAT);
   digitalWrite(RELAY2_PIN, VMC_C_STAT);
   
   // Prepare le JSON
   StaticJsonBuffer<1000> jsonBuffer;
   JsonObject& root = jsonBuffer.createObject();

   root["HUM_L1"]      = HUM_L1;
   root["HUM_L2"]      = HUM_L2;
   root["HUM_R"]       = HUM_R;
   root["TEMP_R"]      = TEMP_R;
   root["VMC_C_STAT"]  = VMC_C_STAT;
   root["VMC_B_TIME"]  = VMC_B_TIME;
   root["VMC_B_STAT"]  = VMC_B_STAT;
   root["PUMP_C_STAT"] = PUMP_C_STAT;
   root["PUMP_B_TIME"] = PUMP_B_TIME;
   root["PUMP_B_STAT"] = PUMP_B_STAT;
   
   char MQTTBUFFER[1000];
   root.printTo(MQTTBUFFER, sizeof(MQTTBUFFER));
   mqtt_publish(MQTT_GET_TOPIC,MQTTBUFFER);
}

void buttonStateChangedISR() { /*  Function called when the button is pressed/released */
  cmd = CMD_BUTTON_STATE_CHANGED;
}

#ifdef PIR
void pirStateChangedISR() { /*    Function called when the PIR sensor detects the biginning/end of a mouvement  */
  cmd = CMD_PIR_STATE_CHANGED;
}
#endif

void tick() { /*   Function called to toggle the state of the LED  */
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void saveConfigCallback () { // callback notifying us of the need to save config
  shouldSaveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  ticker.attach(0.2, tick);
}

void saveconfig(){
    settings.HUM_L1      = HUM_L1;
    settings.HUM_L2      = HUM_L2;
    settings.VMC_B_TIME  = VMC_B_TIME;
    settings.PUMP_B_TIME = PUMP_B_TIME;
    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
}

void pump(){
  if (PUMP_B_STAT==HIGH && PUMP_C_STAT==HIGH){  
       DEBUG_PRINT("Passage en mode force de la POMPE pour ");  DEBUG_PRINTLN(PUMP_B_TIME); 
       PUMP_C_STAT = LOW;      
       setRelayStateandpublish();    
       PUMP_B_COUNT=PUMP_B_TIME;
  }
  if (PUMP_B_COUNT > 0){
      PUMP_B_COUNT--;
      if (PUMP_B_COUNT==0){
        DEBUG_PRINTLN("Passage en mode non force de la POMPE "); 
        PUMP_C_STAT = HIGH; 
        setRelayStateandpublish();    
      }
  }
}

void vmc(){
  if (VMC_B_STAT==HIGH && VMC_C_STAT == HIGH){      
      DEBUG_PRINT("Passage en mode force de la VMC pour ");     DEBUG_PRINTLN(VMC_B_TIME); 
      VMC_C_STAT = LOW;    
      setRelayStateandpublish();
      VMC_B_COUNT=VMC_B_TIME;    
  }
  if (VMC_B_COUNT >0) {       
      VMC_B_COUNT--;
      if (VMC_B_COUNT==0) {
        DEBUG_PRINTLN("Passage en mode non force de la VMC "); 
        VMC_C_STAT = HIGH;     
        setRelayStateandpublish();    
      }
  }
  
  if (VMC_B_STAT==LOW && VMC_B_COUNT == 0){    
    if (HUM_R>HUM_L2 && VMC_C_STAT == HIGH ){  
          VMC_C_STAT  = LOW;                   
          DEBUG_PRINT(F("HUM_R>HUM_L2: "));    DEBUG_PRINT(HUM_R);DEBUG_PRINT(">");  DEBUG_PRINTLN(HUM_L2); 
          setRelayStateandpublish(); 
    }
    if (HUM_R<HUM_L1 && VMC_C_STAT == LOW && HUM_R > 0 ) { 
          VMC_C_STAT  = HIGH;               
          DEBUG_PRINT(F("HUM_R<HUM_L1: "));    DEBUG_PRINT(HUM_R);DEBUG_PRINT("<");  DEBUG_PRINTLN(HUM_L1); 
          setRelayStateandpublish(); 
    }
  }
}

void readbutton(){
  bool VMCreading  = digitalRead(VMC_PIN);  
  bool PUMPreading = digitalRead(PUMP_PIN); 
  
  if (VMCreading != VMClastButtonState) {
    VMClastDebounceTime = millis();
  }
  
  if (PUMPreading != PUMPlastButtonState) {
    PUMPlastDebounceTime = millis();
  }
  if ((millis() - VMClastDebounceTime) > VMCdebounceDelay) {
    if (VMCreading != VMCbuttonState) { // A analyse et créer la variable si necessaire.
        VMCbuttonState = VMCreading;
        VMC_B_STAT  = true;
        //countVMCbuttontime = VMC_B_TIME;
    }else{
        VMC_B_STAT  = false;  
    }
  }
  if ((millis() - PUMPlastDebounceTime) > PUMPdebounceDelay) {
    if (PUMPreading !=    PUMPbuttonState) {
        PUMPbuttonState = PUMPreading;
        PUMP_B_STAT  = true;
        //countPUMPbuttontime = PUMP_B_TIME;
    }else{
        PUMP_B_STAT  = false;
    }
  }
  VMClastButtonState  = VMCreading;
  PUMPlastButtonState = PUMPreading;
}

void prep_MQTT_topic(){
  sprintf(MQTT_CLIENT_ID, "%06X", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT : "));  DEBUG_PRINTLN(MQTT_CLIENT_ID);
  sprintf(MQTT_GET_TOPIC, "%06X/get", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT : "));        DEBUG_PRINTLN(MQTT_GET_TOPIC);
  sprintf(MQTT_SET_TOPIC, "%06X/set", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT : "));        DEBUG_PRINTLN(MQTT_SET_TOPIC);
}
