// Home Assistant Mitsubishi Electric Heat Pump Controller https://github.com/unixko/MitsuCon
// using native MQTT Climate (HVAC) component with MQTT discovery for automatic configuration
// Set PubSubClient.h MQTT_MAX_PACKET_SIZE to 2048
#include "password.h"
#define debug_print                                                // manages most of the print and println debug, not all but most

#if defined(debug_print_serial)
   #define debug_begin(x)        Serial.begin(x)
   #define debug(x)                   Serial.print(x)
   #define debugln(x)                 Serial.println(x)
#elif defined(debug_print_serial1)
   #define debug_begin(x)        Serial1.begin(x)
   #define debug(x)                   Serial1.print(x)
   #define debugln(x)                 Serial1.println(x)
#else
   #define debug_begin(x) 
   #define debug(x)
   #define debugln(x)
#endif


// enable extra MQTT topic for debug/timer info
bool _debugMode  = true;
#define TIMER_ATTR
#define SWING_MODE

// comment out to disable OTA
#define OTA
const char* ota_password  = _ota_password;

// wifi settings
const char* ssid          = _ssid;
const char* password      = _password;

// mqtt server settings
const char* mqtt_server   = _mqtt_server;
const int mqtt_port       = _mqtt_port;
const char* mqtt_username = _mqtt_username;
const char* mqtt_password = _mqtt_password;

// mqtt client settings
// Change "dustsensor" to be same on all lines
const char* name                                = "Dust Sensor"; // Device Name displayed in Home Assistant
const char* client_id                           = "dustsensor"; // WiFi hostname, OTA hostname, MQTT hostname
#define dustsensor_topic                      "dustsensor/{id}"           // MQTT topic, must be unique between heat pump unit
#define dustsensor_availability_topic         "dustsensor/{id}/tele/avty"
#define dustsensor_current_topic              "dustsensor/{id}/tele/curr"
#define dustsensor_debug_topic                "dustsensor/{id}/debug"
#define dustsensor_identify_command_topic     "dustsensor/identify"

// Customization
const char* mqtt_discov_prefix          = "homeassistant"; // Home Assistant MQTT Discovery Prefix

// sketch settings
const unsigned int SEND_DATA_INTERVAL_MS = 60000;
