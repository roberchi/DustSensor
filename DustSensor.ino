#define MQTT_MAX_PACKET_SIZE 2048
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ezLED.h>  // ezLED library
#include "DustSensor.h"
#include "SDS011.h"

#define debug_print_serial1 true // debug to Serial1
#define USE_HWSERIAL true


#ifdef OTA
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#endif

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
SDS011 sds;
unsigned long lastTempSend;
const char* controller_sw_version = "20240212";
ezLED blueLed(LED_BUILTIN, CTRL_CATHODE);


// topics with idconst char* dustsensor_id_availability_topic;
const char* clientTopic(const char* topicTemplate) {
  String topic = String(topicTemplate);
  topic.replace("{id}", String(ESP.getChipId()));
  void* presult = malloc(topic.length() + 1);
  // copy
  strcpy((char*)presult, topic.c_str());
  return (char*)presult;
}

const char* dustsensor_id_topic = clientTopic(dustsensor_topic);
const char* dustsensor_id_availability_topic = clientTopic(dustsensor_availability_topic);     
const char* dustsensor_id_current_topic = clientTopic(dustsensor_current_topic);
const char* dustsensor_id_debug_topic = clientTopic(dustsensor_debug_topic);
    

int mqttConnect() {
  // wifi not connected
  if ((WiFi.status() != WL_CONNECTED)) return -1;
  if (mqtt_client.connected()) return 0;

  unsigned long lastTryConnect = 0;
  if (!mqtt_client.connected()) {
    blueLed.blink(100, 250);
    blueLed.loop();
  }

  const String clientName = String(client_id) + "-" + String(ESP.getChipId());

  while (!mqtt_client.connected()) {
    if (lastTryConnect == 0 || millis() > (lastTryConnect + 5000)) {
      lastTryConnect = millis();
      Serial1.println("MQTT try connecting ...");
      if (mqtt_client.connect(clientName.c_str(), mqtt_username, mqtt_password, dustsensor_id_availability_topic, 1, 1, "offline")) {
        mqtt_client.subscribe(dustsensor_identify_command_topic);
        mqtt_client.publish(dustsensor_id_availability_topic, "online", true);
        Serial1.println("MQTT connected");
      } else Serial1.println("MQTT connection failed, retry in 5 sec");
    }
    blueLed.loop();
    delay(100);
    if ((WiFi.status() != WL_CONNECTED)) {
      Serial1.println("Wi-Fi disconnected");
      blueLed.cancel();
      return -1;
    }
  }

  blueLed.cancel();
  return 0;
}

void mqttAutoDiscovery() {
  const String chip_id = String(ESP.getChipId());
  const String mqtt_discov_topic = String(mqtt_discov_prefix) + "/climate/" + chip_id + "/config";
  const char* root_topic = dustsensor_id_topic;
  Serial1.println(root_topic);
  JsonDocument rootDiscovery;

  rootDiscovery["name"] = name;
  rootDiscovery["uniq_id"] = chip_id;
  rootDiscovery["~"] = root_topic;
  rootDiscovery["unit"] = "µg/m³";
  rootDiscovery["avty_t"] = "~/tele/avty";

  rootDiscovery["current"] = "~/tele/curr";
  rootDiscovery["particulate_matter_10_template"] = "{{ value_json.particulate_matter_10 }}";
  rootDiscovery["particulate_matter_2_5_template"] = "{{ value_json.particulate_matter_2_5 }}";
  
  JsonObject device = rootDiscovery.createNestedObject("device");
  device["name"] = name;
  JsonArray ids = device.createNestedArray("ids");
  ids.add(chip_id);
  device["mf"] = "SDS011 Dust Sensor";
  device["mdl"] = "SDS011 Dust Sensor";
  device["sw"] = controller_sw_version;

  char bufferDiscover[2048];
  serializeJson(rootDiscovery, bufferDiscover);

  mqtt_client.setBufferSize(2048);
  if (!mqtt_client.publish(mqtt_discov_topic.c_str(), bufferDiscover, true)) {
    mqtt_client.publish(dustsensor_id_debug_topic, "failed to publish DISCOV topic");
  }
}


// callback heat pump status changed, send new status to MQTT dustsensor_current_topic
// also send timer attribute to MQTT dustsensor_attribute_topic
// finally update MQTT dustsensor_availability_topic to online status
void sendData(float p10Value, float p25Value, int error) {
  JsonDocument rootInfo;

  rootInfo["name"] = name;
  if(error == 0) {
    rootInfo["particulate_matter_10"] = p10Value;
    rootInfo["particulate_matter_2_5"] = p25Value;
  } else {
    mqtt_client.publish(dustsensor_id_debug_topic, "failed read from SDS011 sensor");
    rootInfo["particulate_matter_10"] = "error";
    rootInfo["particulate_matter_2_5"] = "error";
  }

  char bufferInfo[512];
  serializeJson(rootInfo, bufferInfo);

  if (!mqtt_client.publish(dustsensor_id_current_topic, bufferInfo, true)) {
    mqtt_client.publish(dustsensor_id_debug_topic, "failed to publish CURR topic");
  }

  mqtt_client.publish(dustsensor_id_availability_topic, "online", true);
}



void mqttCallback(char* topic, byte* payload, unsigned int length) {
  blueLed.blinkNumberOfTimes(200, 200, 5);
  Serial1.println("MQTT topic received:");
  Serial1.println(topic);
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial1.println("MQTT message received:");
  Serial1.println(message); 

  // IDENTIFY
  if (strcmp(topic, dustsensor_identify_command_topic) == 0) {
    Serial1.println("IDENTIFY command");
    if (strcmp(message, String(ESP.getChipId()).c_str()) == 0)
      blueLed.blinkNumberOfTimes(500, 200, 25);
  } 
}


void setup() {
  Serial1.begin(115200);

  // set WiFi and connect
  Serial1.println("Wi-Fi setup");
  const String hostname = String(client_id) + "-" + String(ESP.getChipId());
  WiFi.hostname(hostname.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial1.println("Wi-Fi connecting...");
  blueLed.blink(250, 250);
  while (WiFi.status() != WL_CONNECTED) {
    blueLed.loop();
    delay(100);
  }
  Serial1.println("Wi-Fi connected");
  blueLed.cancel();
  // set MQTT
  Serial1.println("MQTT setup");
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqttCallback);

#ifdef OTA
  // set OTA
  Serial1.println("OTA setup");
  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.setPassword(ota_password);
  ArduinoOTA.begin();
#endif

  // connect heat pump
  Serial1.println("SDS011 setup");
  sds.begin(&Serial);
  
  lastTempSend = millis();
}

void wifiReconnect() {
  unsigned long lastTryConnect = 0;
  blueLed.blink(250, 250);
  while (WiFi.status() != WL_CONNECTED) {
    if (lastTryConnect == 0 || millis() > (lastTryConnect + 5000)) {
      WiFi.disconnect();
      WiFi.reconnect();
      lastTryConnect = millis();
    } else {
      blueLed.loop();
    }
  }
  blueLed.cancel();
}

// manage the connection with MQTT broker and in case on Wi Fi disconnection try to reconnect it
void handleConnection(){
  while (!mqtt_client.connected()) {
    if (mqttConnect() == -1)  // WiFi disconnected
      wifiReconnect();
    else
      mqttAutoDiscovery();  // on first connect or reconnect send autodiscovery to HA
  }
}

float p10, p25;
int err;
// manage heat pum syncronization and status 
void handleHeatPumpStatus(){
  if (millis() > (lastTempSend + SEND_DATA_INTERVAL_MS)) {
    blueLed.fade(255, 0, 1000);
    err = sds.read(&p25, &p10);
	  sendData(p10, p25, err);
    lastTempSend = millis();
  }
}

// main loop
void loop() {
  handleConnection();
  mqtt_client.loop();
  blueLed.loop();
#ifdef OTA
  ArduinoOTA.handle();
#endif
}
