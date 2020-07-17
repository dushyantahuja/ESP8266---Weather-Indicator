#define NUM_LEDS 17
#define DATA_PIN D2
#define UPDATES_PER_SECOND 60
//#define DEVICE_NAME "TemperatureDisplay4" // Use deviceid.txt on SPIFFS instead
const int FW_VERSION = 8;
const char* fwUrlBase = "http://ahuja.ws/firmware/TemperatureDisplay";
#define GET_VARIABLE_NAME(Variable) (#Variable).cstr()
#define BRIGHTNESS 120

#ifndef DEBUG_PRINT
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif
#endif

// Function Definitions

DEFINE_GRADIENT_PALETTE(temperature_gp){0, 1, 27, 105, 14, 1, 27, 105, 14, 1, 40, 127, 28, 1, 40, 127, 28, 1, 70, 168, 42, 1, 70, 168, 42, 1, 92, 197, 56, 1, 92, 197, 56, 1, 119, 221, 70, 1, 119, 221, 70, 3, 130, 151, 84, 3, 130, 151, 84, 23, 156, 149, 99, 23, 156, 149, 99, 67, 182, 112, 113, 67, 182, 112, 113, 121, 201, 52, 127, 121, 201, 52, 127, 142, 203, 11, 141, 142, 203, 11, 141, 224, 223, 1, 155, 224, 223, 1, 155, 252, 187, 2, 170, 252, 187, 2, 170, 247, 147, 1, 184, 247, 147, 1, 184, 237, 87, 1, 198, 237, 87, 1, 198, 229, 43, 1, 212, 229, 43, 1, 212, 220, 15, 1, 226, 220, 15, 1, 226, 171, 2, 2, 240, 171, 2, 2, 240, 80, 3, 3, 255, 80, 3, 3};

CRGBPalette16 gCurrentPalette(temperature_gp);

void handleNotFound(AsyncWebServerRequest *request);
void colorwaves(CRGB *ledarray, uint16_t numleds, CRGBPalette16 &palette);
void effects();
void showTime(int hr, int mn, int sec);
void handleUpdate(AsyncWebServerRequest *request);
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void getWeather();
void send_configuration_html(AsyncWebServerRequest *request);
void send_configuration_values_html(AsyncWebServerRequest *request);
void send_weather_values_html(AsyncWebServerRequest *request);

CRGBArray<NUM_LEDS> leds;
CRGBSet ledr(leds(0, 7));
CRGBSet ledt(leds(8, NUM_LEDS));
String message;

struct strConfig
{
  bool autolocation;
  bool updatelocation = false;
  double latitude;
  double longitude;
  //String City;
} config;

bool saveConfig(bool defaultValues = false)
{
  if (defaultValues)
  {
    EEPROM.put(20, 0.00);
    EEPROM.put(28, 0.00);
    EEPROM.write(32, 1);
    EEPROM.write(110, 5);
    EEPROM.commit();
    //EEPROM.update();
    return true;
  }
  else
  {
    EEPROM.put(20, config.latitude);
    EEPROM.put(28, config.longitude);
    if (config.autolocation)
      EEPROM.write(32, 1);
    else
      EEPROM.write(32, 0);
    EEPROM.write(110, 5);
    EEPROM.commit();
    //EEPROM.update();
    return true;
  }
}

bool loadDefaults()
{
  if (EEPROM.read(110) != 5)
  {
    config.autolocation = true;
    saveConfig( true );
    DEBUG_PRINT("Defaults Loaded");
  }
  else
  {
    DEBUG_PRINT("EEPROM Loading");
    EEPROM.get(20, config.latitude);
    EEPROM.get(28, config.longitude);
    byte autolocation = EEPROM.read(32);
    if (autolocation == 1)
      config.autolocation = true;
    else
      config.autolocation = false;
    DEBUG_PRINT("EEPROM Loaded");
  }
  DEBUG_PRINT(String(config.autolocation));
  DEBUG_PRINT(String(config.latitude));
  DEBUG_PRINT(String(config.longitude));
  return true;
}

// Code from https://github.com/lbernstone/asyncUpdate/blob/master/AsyncUpdate.ino

void handleUpdate(AsyncWebServerRequest *request)
{
  const char *html = "<form method='POST' action='/doUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
  request->send(200, "text/html", html);
}

void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    DEBUG_PRINT("Update");
    size_t content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
#ifdef ESP8266
    Update.runAsync(true);
    if (!Update.begin(content_len, cmd))
    {
#else
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
    {
#endif
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len)
  {
    //Update.printError(Serial);
#ifdef ESP8266
  }
  else
  {
    //Serial.printf("Progress: %d%%\n", (Update.progress()*100)/Update.size());
#endif
  }

  if (final)
  {
    if (!Update.end(true))
    {
      Update.printError(Serial);
    }
    else
    {
      DEBUG_PRINT("Update complete");
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Please wait while the device reboots");
      response->addHeader("Refresh", "15; url=/");
      //response->addHeader("Location", "/");
      response->addHeader("Connection", "close");
      request->send(response);
      Serial.flush();
      //delay(200);
      ESP.restart();
    }
  }
}

// FastLED colorwaves https://gist.github.com/kriegsman/8281905786e8b2632aeb

void colorwaves(CRGB *ledarray, uint16_t numleds, CRGBPalette16 &palette)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  //uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16; //gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis;
  sLastMillis = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0; i < numleds; i++)
  {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if (h16_128 & 0x100)
    {
      hue8 = 255 - (h16_128 >> 1);
    }
    else
    {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8(index, 240);

    CRGB newcolor = ColorFromPalette(palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds - 1) - pixelnumber;

    nblend(ledarray[pixelnumber], newcolor, 128);
  }
}

// Based on information from https://www.hackster.io/detox/send-esp8266-data-to-your-webpage-no-at-commands-7ebfec?f=1#code

void sendIP()
{
  WiFiClient client;
  HTTPClient http;
  String url_ahuja = "http://ahuja.ws/esp.php?ESP=" + String(DEVICE_NAME) + "&IP=" + WiFi.localIP().toString();
  http.begin(client, url_ahuja);
  http.GET();
  http.end();
}

//Modified using code from https://www.bakke.online/index.php/2017/06/02/self-updating-ota-firmware-for-esp8266/

void checkForUpdates() {
  //String mac = getMAC();
  String fwURL = String( fwUrlBase );
  //fwURL.concat( mac );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  Serial.println( "Checking for firmware updates." );
  //Serial.print( "MAC address: " );
  //Serial.println( mac );
  Serial.print( "Firmware version URL: " );
  Serial.println( fwVersionURL );

  WiFiClient client;
  HTTPClient httpClient;
  httpClient.begin(client, fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    Serial.print( "Current firmware version: " );
    Serial.println( FW_VERSION );
    Serial.print( "Available firmware version: " );
    Serial.println( newFWVersion );

    int newVersion = newFWVersion.toInt();

    if( newVersion > FW_VERSION ) {
      Serial.println( "Preparing to update" );

      String fwImageURL = fwURL;
      fwImageURL.concat( ".bin" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( client, fwImageURL );

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK");
          break;
      }
    }
    else {
      Serial.println( "Already on latest version" );
    }
  }
  else {
    Serial.print( "Firmware version check failed, got HTTP response code " );
    Serial.println( httpCode );
  }
  httpClient.end();
}