#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

byte mac[6];
unsigned long lastMillis = 0;
bool an = 0;
char json[200];

// Account data 
const char* influx = "192.168.0.2";
const char* mqtt_server = "192.168.0.2";
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWD";

// Led config

#define DATA_PIN     D5
#define NUM_LEDS    6
CRGB leds[NUM_LEDS];


// Clients
WiFiClient client;
PubSubClient  mqttclient(client);



void mqttconnect() {
    // Loop until we're reconnected
    while (!mqttclient.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Create a random client ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (mqttclient.connect(clientId.c_str())) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        mqttclient.publish("outTopic", "hello world");
        // ... and resubscribe
        mqttclient.subscribe("inOnAir");
      } else {
        Serial.print("failed, rc=");
        Serial.print(mqttclient.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  }

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    an = 1;
      for(int dot = 0; dot < NUM_LEDS; dot++) { 
        leds[dot] = CRGB::Red;
      }   
    FastLED.show(); 

  } else {
    an = 0;
    for(int dot = 0; dot < NUM_LEDS; dot++) { 
        leds[dot] = CRGB::Black;
      }   
    FastLED.show(); 
  }

}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  randomSeed(micros());

  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setCallback(callback);
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("feuchte-regal-unten");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  WiFi.macAddress(mac);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

void loop() {
  if (millis() - lastMillis > 2000) {
    DynamicJsonDocument root(200);
    root["an"] = an;
    serializeJson(root, json);
    Serial.print(an);
    Serial.println(" Sent ");
    serializeJson(root, Serial);
    mqttclient.publish("onair", json);

    lastMillis = millis();
  }
  
  ArduinoOTA.handle();

  

  if (!mqttclient.connected()) {
    mqttconnect();
  } else {
  //  Serial.println("Still connected");
  }
  if (!mqttclient.loop()) {
    mqttconnect();
  }
}
