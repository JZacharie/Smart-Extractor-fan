
#define DHTSENSOR
#ifdef DHTSENSOR
  #include <DHT.h>
  #include <DHT_U.h>
  #define DHTPIN            2         // Pin which is connected to the DHT sensor.
  #define DHTTYPE DHT22
  //DHT22 Sensor
  uint32_t delayMS;                   //Readtime for DHT22
  int nbread = 60;                    // ToreducethepublicationonMQTT
  DHT_Unified dht(DHTPIN, DHTTYPE);
  String temperature, humidite; //for string add
  char *temperaturec; //Value compatible only with MQTT
  char *humiditec;    //Value compatible only with MQTT
  float humiditySet1; // stop  VMC when lower than xx%
  float humiditySet2; // Start VMC when upper than yy%
  float savehumiditySet2;
  float floatreadhumidite;
#endif

  int VMCreading  = digitalRead(VMC_PIN);
  int PUMPreading = digitalRead(PUMP_PIN);

int  VMCbuttonState     = 0; // HIGH: opened switch
//bool VMCforcedMODE    = false;
int VMCbuttontime       = 0;
int countVMCbuttontime  = 0;

int  PUMPbuttonState    = 0; // HIGH: opened switch
//bool PUMPforcedMODE   = false;
int  PUMPbuttontime     = 0;
int countPUMPbuttontime = 0;

#define           LED
#ifdef LED
    #include <Adafruit_NeoPixel.h>
    #ifdef __AVR__
      #include <avr/power.h>
    #endif
    // Which pin on the Arduino is connected to the NeoPixels?
    // On a Trinket or Gemma we suggest changing this to 1

    // When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
    // Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
    // example for more information on possible values.
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
char              MQTT_SWITCH_STATE_TOPIC1[STRUCT_CHAR_ARRAY_SIZE]   = {0};
char              MQTT_SWITCH_STATE_TOPIC2[STRUCT_CHAR_ARRAY_SIZE]   = {0};
char              MQTT_STAT_TEMP[STRUCT_CHAR_ARRAY_SIZE]             = {0};
char              MQTT_STAT_HUMIDITY[STRUCT_CHAR_ARRAY_SIZE]         = {0};
char              MQTT_GET_HUMIDITY1[STRUCT_CHAR_ARRAY_SIZE]         = {0};
char              MQTT_GET_HUMIDITY2[STRUCT_CHAR_ARRAY_SIZE]         = {0};
char              MQTT_SET_HUMIDITY1[STRUCT_CHAR_ARRAY_SIZE]         = {0};
char              MQTT_SET_HUMIDITY2[STRUCT_CHAR_ARRAY_SIZE]         = {0};

char              MQTT_SET_DELAY_PUMP[STRUCT_CHAR_ARRAY_SIZE]         = {0};
char              MQTT_GET_DELAY_PUMP[STRUCT_CHAR_ARRAY_SIZE]         = {0};
char              MQTT_SET_DELAY_VMC[STRUCT_CHAR_ARRAY_SIZE]          = {0};
char              MQTT_GET_DELAY_VMC[STRUCT_CHAR_ARRAY_SIZE]          = {0};

char              MQTT_STAT_ERROR[STRUCT_CHAR_ARRAY_SIZE]            = {0};
char              MQTT_SWITCH_COMMAND_TOPIC1[STRUCT_CHAR_ARRAY_SIZE] = {0};
char              MQTT_SWITCH_COMMAND_TOPIC2[STRUCT_CHAR_ARRAY_SIZE] = {0};



char              mqtt_server[STRUCT_CHAR_ARRAY_SIZE] = {0};
char              mqtt_user[STRUCT_CHAR_ARRAY_SIZE] = {0};
char              mqtt_password[STRUCT_CHAR_ARRAY_SIZE] = {0};
char              mqtt_port[STRUCT_CHAR_ARRAY_SIZE] = {0};

char*             MQTT_SWITCH_ON_PAYLOAD                             = "On";
char*             MQTT_SWITCH_OFF_PAYLOAD                            = "Off";

    char charstrhumiditySet1[50];
    char charstrhumiditySet2[50];
    char charVMCbuttontime[50];
    char charPUMPbuttontime[50];
    
    String strhumiditySet1;
    String strhumiditySet2;
    String strVMCbuttontime;
    String strPUMPbuttontime;
    

// Settings for VMC
typedef struct {
  char            mqttUser[STRUCT_CHAR_ARRAY_SIZE]                  = "";//{0};
  char            mqttPassword[STRUCT_CHAR_ARRAY_SIZE]              = "";//{0};
  char            mqttServer[STRUCT_CHAR_ARRAY_SIZE]                = "";//{0};
  char            mqttPort[6]                                       = "";//{0};
  char            humiditySet1[6]                                   = "";//{0};
  char            humiditySet2[6]                                   = "";//{0};
  char            PUMPbuttontime[STRUCT_CHAR_ARRAY_SIZE]            = "5000";//{0};
  char            VMCbuttontime[STRUCT_CHAR_ARRAY_SIZE]             = "5000";//{0};
} Settings;

enum CMD {
  CMD_NOT_DEFINED,
  CMD_PIR_STATE_CHANGED,
  CMD_BUTTON_STATE_CHANGED,
};

volatile uint8_t cmd = CMD_NOT_DEFINED;

uint8_t           relayState1                                       = HIGH; // HIGH: closed switch
uint8_t           relayState2                                       = HIGH; // HIGH: closed switch
uint8_t           buttonState                                       = HIGH; // HIGH: opened switch
uint8_t           currentButtonState                                = buttonState;
long              buttonStartPressed                                = 0;
long              buttonDurationPressed                             = 0;
uint8_t           pirState                                          = LOW; 
uint8_t           currentPirState                                   = pirState;

uint8_t           VMCforcedMODE                                     = LOW; 
int               VMClastButtonState                                = LOW;      // the previous reading from the input pin
long              VMClastDebounceTime                               = 0;        // the last time the output pin was toggled
long              VMCdebounceDelay                                  = 2;        // the debounce time; increase if the output flickers

uint8_t           PUMPforcedMODE                                    = LOW; 
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

/*    Function called to restart the switch  */
void restart() {
  DEBUG_PRINTLN(F("INFO: Restart..."));
    ESP.reset();
  delay(1000);
}

/*    Function called to reset the configuration of the switch  */
void reset() {
  DEBUG_PRINTLN(F("INFO: Reset..."));
  //WiFi.disconnect();
  ESP.reset();
  delay(1000);
}

/*  Function called to connect/reconnect to the MQTT broker  */
void reconnect() {
  uint8_t i = 0;
  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, settings.mqttUser, settings.mqttPassword)) {
      DEBUG_PRINTLN(F("INFO: The client is successfully connected to the MQTT broker"));
    } else {
      DEBUG_PRINTLN(F("ERROR: The connection to the MQTT broker failed"));
      DEBUG_PRINT(F("Username: "));      DEBUG_PRINTLN(settings.mqttUser);
      DEBUG_PRINT(F("Broker: "));        DEBUG_PRINTLN(settings.mqttServer);
      delay(10);
      if (i == 3) {
        reset();
      }
      i++;
    }
  }


//Subscribe MQTT
  if (mqttClient.subscribe(MQTT_SWITCH_COMMAND_TOPIC1)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));  DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC1);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed. Topic: "));    DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC1);
  }
  
  if (mqttClient.subscribe(MQTT_SWITCH_COMMAND_TOPIC2)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));  DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC2);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed. Topic: "));    DEBUG_PRINTLN(MQTT_SWITCH_COMMAND_TOPIC2);
  }

  if (mqttClient.subscribe(MQTT_SET_HUMIDITY1)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));  DEBUG_PRINTLN(MQTT_SET_HUMIDITY1);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed. Topic: "));  DEBUG_PRINTLN(MQTT_SET_HUMIDITY1);
  }
  
  if (mqttClient.subscribe(MQTT_SET_HUMIDITY2)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));  DEBUG_PRINTLN(MQTT_SET_HUMIDITY2);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed. Topic: "));    DEBUG_PRINTLN(MQTT_SET_HUMIDITY2);
  }

  if (mqttClient.subscribe(MQTT_SET_DELAY_PUMP)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));  DEBUG_PRINTLN(MQTT_SET_DELAY_PUMP);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed.   Topic: "));  DEBUG_PRINTLN(MQTT_SET_DELAY_PUMP);
  }
  
  if (mqttClient.subscribe(MQTT_SET_DELAY_VMC)) {
    DEBUG_PRINT(F("INFO: Sending the MQTT subscribe succeeded. Topic: "));  DEBUG_PRINTLN(MQTT_SET_DELAY_VMC);
  } else {
    DEBUG_PRINT(F("ERROR: Sending the MQTT subscribe failed. Topic: "));    DEBUG_PRINTLN(MQTT_SET_DELAY_VMC);
  }

}

void initDHTSENSOR(){
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
}

void mqtt_publish(char MQTT_BUFFER_TOPIC[STRUCT_CHAR_ARRAY_SIZE]  , char*    MQTT_BUFFER_PAYLOAD  ) {
    if (mqttClient.publish(MQTT_BUFFER_TOPIC, MQTT_BUFFER_PAYLOAD , true)) {
      DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));      DEBUG_PRINT(MQTT_BUFFER_TOPIC);   DEBUG_PRINT(F(" : Payload: "));                                          DEBUG_PRINTLN(MQTT_BUFFER_PAYLOAD);
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

/*  Function called to verify the fingerprint of the MQTT server certificate */
void verifyFingerprint() {
  DEBUG_PRINT(F("INFO: Connecting to "));  DEBUG_PRINTLN(settings.mqttServer);

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

/*  Function called to publish the state of the Sonoff relay */
void publishSwitchState() {
  if (relayState1 == HIGH) {
    mqtt_publish(MQTT_SWITCH_STATE_TOPIC1, MQTT_SWITCH_ON_PAYLOAD);
  } else {
    mqtt_publish(MQTT_SWITCH_STATE_TOPIC2, MQTT_SWITCH_OFF_PAYLOAD);
  }
}

void publishSwitchState2() {
  if (relayState2 == HIGH) {
    mqtt_publish(MQTT_SWITCH_STATE_TOPIC2, MQTT_SWITCH_ON_PAYLOAD);
  } else {
    mqtt_publish(MQTT_SWITCH_STATE_TOPIC2, MQTT_SWITCH_OFF_PAYLOAD);
  }
}

/*  Function called to set the state of the relay  */

#ifdef NODEMCU 
  void setRelayState1() {
    digitalWrite(RELAY_PIN, !relayState1);
    publishSwitchState();
  }

  void setRelayState2() {
    digitalWrite(RELAY2_PIN, !relayState2);
    publishSwitchState2();
  }
#else //Inverted for SONOF
  void setRelayState1() {
    digitalWrite(RELAY_PIN, !relayState1);
    digitalWrite(LED_PIN, (relayState1 + 1) % 2);
    publishSwitchState();
  }
  void setRelayState2() {
    digitalWrite(RELAY2_PIN, !relayState2);
    publishSwitchState2();
  }
#endif


/*  Function called when the button is pressed/released */
void buttonStateChangedISR() {
  cmd = CMD_BUTTON_STATE_CHANGED;
}

/*    Function called when the PIR sensor detects the biginning/end of a mouvement  */
 #ifdef PIR
void pirStateChangedISR() {
  cmd = CMD_PIR_STATE_CHANGED;
}
#endif




/*   Function called to toggle the state of the LED  */
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

void saveconfig(){
    char charstrhumiditySet1[50];
    char charstrhumiditySet2[50];
    char charVMCbuttontime[50];
    char charPUMPbuttontime[50];

    DEBUG_PRINTLN("---------Start Saving configue");
    
    strhumiditySet1=String(humiditySet1);
    strhumiditySet2=String(humiditySet2);
    strVMCbuttontime=String(VMCbuttontime);
    strPUMPbuttontime=String(PUMPbuttontime);
    
    strhumiditySet1.toCharArray  (charstrhumiditySet1, 50); 
    strhumiditySet2.toCharArray  (charstrhumiditySet2, 50);
    strVMCbuttontime.toCharArray (charVMCbuttontime,   50);
    strPUMPbuttontime.toCharArray(charPUMPbuttontime,  50); 
      
    strcpy(settings.humiditySet1,   charstrhumiditySet1);
    strcpy(settings.humiditySet2,   charstrhumiditySet2);
    strcpy(settings.VMCbuttontime,  charVMCbuttontime);
    strcpy(settings.PUMPbuttontime, charPUMPbuttontime);
    
    DEBUG_PRINT(F("settings.mqttServer: "));       DEBUG_PRINTLN(settings.mqttServer);
    DEBUG_PRINT(F("settings.mqttUser: "));         DEBUG_PRINTLN(settings.mqttUser);
    //DEBUG_PRINT(F("settings.mqttPassword: "));   DEBUG_PRINTLN(settings.mqttPassword);
    DEBUG_PRINT(F("settings.mqttPort: "));         DEBUG_PRINTLN(settings.mqttPort);
    DEBUG_PRINT(F("settings.humiditySet1: "));     DEBUG_PRINTLN(settings.humiditySet1);
    DEBUG_PRINT(F("settings.humiditySet2: "));     DEBUG_PRINTLN(settings.humiditySet2);            
    DEBUG_PRINT(F("settings.PUMPbuttontime: "));   DEBUG_PRINTLN(settings.PUMPbuttontime);
    DEBUG_PRINT(F("settings.VMCbuttontime: "));    DEBUG_PRINTLN(settings.VMCbuttontime);        
    
    savehumiditySet2=humiditySet2; // For force Mode
    
    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
    DEBUG_PRINTLN("---------END Saving configue");
}


