#include "Arduino.h"
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Updater.h>

#include <ESPAsyncWiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <SPIFFSEditor.h>

AsyncWebServer httpServer(80);
DNSServer dns;

#define DEBUG

#include <NTPClient.h>

// NTP Servers:

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 360000); //19800

//#include <ArduinoJson.h>
#include <IPGeolocation.h>
String IPGeoKey = "b294be4d4a3044d9a39ccf42a564592b";

#include "SimpleWeather.h"
String DKey = "3b1e2d14449b6250a5c77364a1355079"; //"411755509e9a5b74a4b5dfe1ffd91f53";
weatherData w[2];

#define FASTLED_INTERNAL
#define FASTLED_ESP8266_RAW_PIN_ORDER
//#define FASTLED_ESP8266_D1_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
CRGB r[3], t[3];
CRGBPalette16 tpalette, rpalette;

#include "EEPROM.h"

//#include "palette.h"
#include "config.h"
#include "Page_Admin.h"

void setup()
{
  // put your setup code here, to run once:
  delay(3000);
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  //fill_rainbow(leds, NUM_LEDS, 4);
  //FastLED.setBrightness(BRIGHTNESS);
  //FastLED.show();
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  DEBUG_PRINT("Wifi Setup Initiated");
  WiFi.setAutoConnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  AsyncWiFiManager wifiManager(&httpServer, &dns);
  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect(DEVICE_NAME))
  {
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  DEBUG_PRINT("Wifi Setup Completed");

  MDNS.begin(DEVICE_NAME);
  MDNS.addService("http", "tcp", 80);

  // Admin page
  httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", PAGE_AdminMainPage);
  });
  httpServer.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/style.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer.on("/microajax.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/microajax.js.gz", "text/plain");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer.on("/jscolor.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/jscolor.js.gz", "text/plain");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  httpServer.on("/config.html", HTTP_GET, send_configuration_html);
  httpServer.on("/admin/config", HTTP_GET, send_configuration_values_html);
  httpServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) { handleUpdate(request); });
  httpServer.on(
      "/doUpdate", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
         size_t len, bool final) { handleDoUpdate(request, filename, index, data, len, final); });
  httpServer.addHandler(new SPIFFSEditor("admin", "admin"));
  httpServer.onNotFound(handleNotFound);
  httpServer.begin();

  sendIP();
  EEPROM.begin(512);
  loadDefaults();

  if (config.autolocation)
  {
    IPGeolocation IPG(IPGeoKey);
    IPGeo I;
    IPG.updateStatus(&I);
    DEBUG_PRINT(I.city);
    config.latitude = I.latitude;   //51.49580; //
    config.longitude = I.longitude; //-0.22425; //
  }
  getWeather();
  timeClient.setTimeOffset(w[0].tz * 3600);
  timeClient.begin();
  timeClient.update();
  //wdt_enable(WDTO_8S);
}

uint8_t step = 255;

void loop()
{
  timeClient.update();
  MDNS.update();
  EVERY_N_MILLISECONDS(1000 / UPDATES_PER_SECOND)
  {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    if (timeClient.getMinutes() % 2 == 0)
    { //Show Current
      ledr[0] = CRGB::Red;
      ledr[1] = CRGB::Black;
      if (w[0].weather == "snow")
        ledr[3] = CRGB::Violet;
      else if (w[0].rain >= 0.7)
        ledr[4] = CRGB::Blue;
      else if (w[0].cloud >= 0.6)
        ledr[5] = CRGB::LightCyan;
      else if (w[0].cloud >= 0.3)
        ledr[6] = CRGB::LightGreen;
      else
        ledr[7] = CRGB::Green;
      int i = w[0].current_Temp / 5;
      if (i < 0)
        i = 0;
      colorwaves(ledt, i, gCurrentPalette);
    }
    else
    { //Show Forecast
      ledr[1] = CRGB::Red;
      ledr[0] = CRGB::Black;
      if (w[1].weather == "snow")
        ledr[3] = CRGB::Violet;
      else if (w[1].rain >= 0.7)
        ledr[4] = CRGB::Blue;
      else if (w[1].cloud >= 0.6)
        ledr[5] = CRGB::LightCyan;
      else if (w[1].cloud >= 0.3)
        ledr[6] = CRGB::LightGreen;
      else
        ledr[7] = CRGB::Green;
      int i = w[1].current_Temp / 5;
      if (i < 0)
        i = 0;
      colorwaves(ledt, i, gCurrentPalette);
    }
    FastLED.show();
  }
  if (timeClient.getHours() >= 22 || timeClient.getHours() < 7)
  {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
  }
  else
  {
    if (timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0)
      getWeather(); //Get weather update every hour.
    else if ((timeClient.getMinutes() % 5) == 0 && timeClient.getSeconds() == 0)
      effects();
  }
  if (timeClient.getHours() == 3 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0)
    ESP.restart();
  yield();
}

void getWeather()
{

  for (int i = 0; i < 2; i++)
  {
    DEBUG_PRINT("UnixTime: " + String(timeClient.getEpochTime()));
    DEBUG_PRINT("Co-ordinates: " + String(config.latitude) + ":" + String(config.longitude));
    OpenWeather ds(DKey, config.latitude, config.longitude, i != 0);
    //ds.updateURL(DKey, config.latitude, config.longitude, timeClient.getEpochTime() + i * 3600 * 3); // Get data for every 3 hours
    ds.updateStatus(&w[i]);
    DEBUG_PRINT("Temp: ");
    DEBUG_PRINT(String(w[i].current_Temp));
    DEBUG_PRINT("Rain: ");
    DEBUG_PRINT(String(w[i].rain));
    DEBUG_PRINT("Icon: ");
    DEBUG_PRINT(w[i].weather);
    message += "---------------\n";
    message += "Current Temperature: " + String(w[i].current_Temp) + "\n";
    message += "Rainfall Probability: " + String(w[i].rain) + "\n";
    message += "Weather: " + w[i].weather + "\n";
    message += "---------------\n";
    yield();
    delay(500);
  }
}

void effects()
{
  for (int j = 0; j < 300; j++)
  {
    fadeToBlackBy(leds, NUM_LEDS, 20);
    byte dothue = 0;
    for (int i = 0; i < 8; i++)
    {
      leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  message += "Current Temperature: " + String(w[1].current_Temp) + "\n";
  message += "Rainfall Probability: " + String(w[1].rain) + "\n";
  message += "Weather: " + w[1].weather + "\n";
  message += "Time: ";
  message += String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds()) + "\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->params();
  message += "\n";
  for (uint8_t i = 0; i < request->params(); i++)
  {
    AsyncWebParameter *p = request->getParam(i);
    message += " " + p->name() + ": " + p->value() + "\n";
  }
  request->send(404, "text/plain", message);
  message = "";
}

void send_configuration_values_html(AsyncWebServerRequest *request)
{
  String values = "";
  values += "latitude|" + String(config.latitude) + "|input\n";
  values += "longitude|" + String(config.longitude) + "|input\n";
  if (config.autolocation)
    values += "autolocation|true|chk";
  request->send(200, "text/plain", values);
}

void send_configuration_html(AsyncWebServerRequest *request)
{
  if (request->args() > 0) // Save Settings
  {
    if (request->hasParam("latitude"))
    {
      AsyncWebParameter *p = request->getParam("latitude");
      config.latitude = strtol(p->value().c_str(), NULL, 16);
      EEPROM.put(20, config.latitude);
      /*EEPROM.write(6, hours.r);
      EEPROM.write(7, hours.g);
      EEPROM.write(8, hours.b);
      EEPROM.commit();*/
    }
    if (request->hasParam("longitude"))
    {
      AsyncWebParameter *p = request->getParam("longitude");
      config.longitude = strtol(p->value().c_str(), NULL, 16);
      EEPROM.put(28, config.longitude);
      /*EEPROM.write(6, hours.r);
      EEPROM.write(7, hours.g);
      EEPROM.write(8, hours.b);
      EEPROM.commit();*/
    }
    if (request->hasParam("autolocation"))
    {
      AsyncWebParameter *p = request->getParam("autolocation");
      if (p->value() == "true")
      {
        config.autolocation = true;
        IPGeolocation IPG(IPGeoKey);
        IPGeo I;
        IPG.updateStatus(&I);
        DEBUG_PRINT(I.city);
        config.latitude = I.latitude;   //51.49580; //
        config.longitude = I.longitude; //-0.22425; //
      }
      else
        config.autolocation = false;
      EEPROM.put(32, config.autolocation);
      /*EEPROM.write(6, hours.r);
      EEPROM.write(7, hours.g);
      EEPROM.write(8, hours.b);
      EEPROM.commit();*/
    }
    getWeather();
  }
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/www/config.html", "text/html");
  //response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}
