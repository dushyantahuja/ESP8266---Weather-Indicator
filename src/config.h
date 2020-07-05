#define NUM_LEDS 26
#define DATA_PIN D4
#define UPDATES_PER_SECOND 40
#define DEVICE_NAME "TemperatureDisplay"
#define GET_VARIABLE_NAME(Variable) (#Variable).cstr()
#define BRIGHTNESS 180

#ifndef DEBUG_PRINT
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif
#endif

// Function Definitions

void handleNotFound(AsyncWebServerRequest *request);
void colorwaves(CRGB *ledarray, uint16_t numleds, CRGBPalette16 &palette);
void effects();
void showTime(int hr, int mn, int sec);
void handleUpdate(AsyncWebServerRequest *request);
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void getWeather();
void send_configuration_html(AsyncWebServerRequest *request);
void send_configuration_values_html(AsyncWebServerRequest *request);

CRGBArray<NUM_LEDS> leds;
CRGBSet ledt(leds(0, NUM_LEDS / 2 - 1));
CRGBSet ledr(leds(NUM_LEDS / 2, NUM_LEDS));
String message;

struct strConfig
{
  bool autolocation;
  double latitude;
  double longitude;
  //String City;
} config;

bool saveDefaults()
{
  { // Check if colours have been set or not
    /*

      EEPROM.write(12, 0);                   // Light sensitivity - low
      EEPROM.write(13, 65);                  // Light sensitivity - high
      EEPROM.write(14, 30);                  // Minutes for each rainbow
      EEPROM.write(15, 2);                    // Current Palette
      EEPROM.write(16,22);                    // Switch Off
      EEPROM.write(17,7);                     // Switch On
      EEPROM.write(18,64);                     //lines colour
      EEPROM.write(19,64);
      EEPROM.write(20,50);
      EEPROM.write(109,4);
      EEPROM.commit();*/
  }
  return true;
}

bool loadDefaults()
{
  config.latitude = 51.5074;
  config.longitude = 0.1278;
  config.autolocation = true;
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