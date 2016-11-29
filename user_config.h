// PIN properties

// static const uint8_t    N/A          = D0; // 16
static const uint8_t     STRIP_PIN      = D1; // 5  
static const uint8_t     RELAY2_PIN     = D2; // 4 
static const uint8_t     BUTTON_PIN     = D3; // 0  // SONOFF 
static const uint8_t     DHT_PIN        = D4; // 2 
static const uint8_t     VMC_PIN        = D5; // 14 // SONOFF 
static const uint8_t     RELAY_PIN      = D6; // 12 
static const uint8_t     LED_PIN        = D7; // 13 // SONOFF 
static const uint8_t     PUMP_PIN       = D8; // 15

static const uint8_t     NUMPIXELS      = 4;       // Number of Pixel or Led Strip

String strmqtt_server   = "";
String strmqtt_user     = "";
String strmqtt_password = "";
String strmqtt_port     = "";
  
// SHA1 fingerprint of the certificate // openssl x509 -fingerprint -in  <certificate>.crt
const char*       fingerprint = "DB 0E E4 F4 46 CD EA 92 A3 6C 84 37 DF B6 B9 65 ED 89 3F 2B";

//NODEMCU or SONOFF
#define NODEMCU
//#define SONOFF


