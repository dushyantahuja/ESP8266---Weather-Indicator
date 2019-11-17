#include "Arduino.h"
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Updater.h>

#include <ESPAsyncWiFiManager.h>
#include <ESP8266mDNS.h>
#include <SPIFFSEditor.h>

AsyncWebServer httpServer(80);
DNSServer dns;

#define DEBUG

#include <NTPClient.h>
//#include <ArduinoJson.h>
#include <IPGeolocation.h>
String IPGeoKey = "b294be4d4a3044d9a39ccf42a564592b";

#include "SimpleWeather.h"
String DKey = "411755509e9a5b74a4b5dfe1ffd91f53";


#define FASTLED_INTERNAL
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include "EEPROM.h"

//#include "palette.h"
#include "config.h"
#include "Page_Admin.h"

// NTP Servers:

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 360000); //19800

void setup() {
    // put your setup code here, to run once:
    delay(3000);
    Serial.begin(9600);
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    fill_rainbow(leds, NUM_LEDS,4);
    FastLED.setBrightness(20);
    FastLED.show();
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
    Serial.println("Wifi Setup Initiated");
    WiFi.setAutoConnect(true);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    AsyncWiFiManager wifiManager(&httpServer,&dns);
    wifiManager.setTimeout(180);
    if(!wifiManager.autoConnect("DEVICE_NAME")) {
      delay(3000);
      ESP.reset();
      delay(5000);
      }
    Serial.println("Wifi Setup Completed");
    MDNS.begin("DEVICE_NAME");
    MDNS.addService("http", "tcp", 80);

    // Admin page
    httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request ->send_P(200,"text/html", PAGE_AdminMainPage ); 
    });
    httpServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/style.css.gz", "text/css");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/microajax.js", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/microajax.js.gz", "text/plain");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/jscolor.js", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/jscolor.js.gz","text/plain");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    httpServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){handleUpdate(request);});
    httpServer.on("/doUpdate", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                  size_t len, bool final) {handleDoUpdate(request, filename, index, data, len, final);}
    );
    httpServer.addHandler(new SPIFFSEditor("admin","admin"));
    httpServer.onNotFound(handleNotFound);
    httpServer.begin();

    IPGeolocation IPG(IPGeoKey);
    IPGeo I;
    IPG.updateStatus(&I);
    config.latitude = I.latitude; //51.49580; //
    config.longitude = I.longitude; //-0.22425; //
    timeClient.setTimeOffset(I.offset*3600);

    MDNS.addService("http", "tcp", 80);
    timeClient.begin();

    EEPROM.begin(512);

    if (EEPROM.read(109) != 4) saveDefaults();
    // Else read the parameters from the EEPROM
    else loadDefaults();
    getWeather();
    wdt_enable(WDTO_8S);
}

void loop() {
    timeClient.update();
    if(timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0) getWeather(); //Get weather update every hour. 
    //FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
    if(timeClient.getHours() == 3 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0)
      ESP.restart();
    yield();
}

void getWeather() {
  weatherData w;
  CRGB t[3],r[3];
  Darksky ds(DKey,config.latitude,config.longitude);
  for(int i = 0; i < 1; i++){
    DEBUG_PRINT(String(timeClient.getEpochTime()));
    ds.updateURL(DKey,config.latitude,config.longitude,timeClient.getEpochTime()+i*3600);
    ds.updateStatus(&w);
    DEBUG_PRINT("Temp: ");
    DEBUG_PRINT(String(w.current_Temp));
    DEBUG_PRINT("Rain: ");
    DEBUG_PRINT(String(w.rain));
    DEBUG_PRINT("Icon: ");
    DEBUG_PRINT(w.weather);
    if(w.current_Temp < 5) t[i] = CRGB::BlueViolet;       
      else if (w.current_Temp <10) t[i] = CRGB::DarkBlue;
      else if (w.current_Temp <15) t[i] = CRGB::Blue;
      else if (w.current_Temp <20) t[i] = CRGB::Green;
      else if (w.current_Temp <25) t[i] = CRGB::Orange; 
      else t[i] = CRGB::OrangeRed; 
    if(w.weather == "snow") r[i] = CRGB::Violet;
      else if (w.rain >= 0.7) r[i] = CRGB::Blue;
      else if (w.weather == "cloudy") r[i] = CRGB::LightBlue;
      else r[i] = CRGB::Green;
    //message += "Darksky Response:\n";
    //message += ds.getResponse().c_str();
    FastLED.show();
    yield();
    delay(500);
  }
  for(int i = 0; i < 3; i++){
    for(int j = 0; j < 10; j++){
      leds[i*10+j]=t[i];
      leds[i*10+j+30]=r[i];
      //delay(20);
    }
  }  
}

void effects(){
  for( int j = 0; j< 300; j++){
    fadeToBlackBy( leds, NUM_LEDS, 20);
    byte dothue = 0;
    for( int i = 0; i < 8; i++) {
      leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
    FastLED.show();
    FastLED.delay(1000/UPDATES_PER_SECOND);
   }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void handleNotFound(AsyncWebServerRequest *request){
  message+= "Time: ";
  message+= String(timeClient.getHours()) + ":" + String(timeClient.getMinutes())+ ":" + String(timeClient.getSeconds()) + "\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += request->params();
  message += "\n";
  for (uint8_t i=0; i<request->params(); i++){
    AsyncWebParameter* p = request->getParam(i);
    message += " " + p->name() + ": " + p->value() + "\n";
  }
  request->send(404, "text/plain", message);
  message = "";
}

