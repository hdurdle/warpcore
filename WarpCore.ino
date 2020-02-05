#include <ESP8266WiFi.h>
#include <FastLED.h>
#include "config.h" // edit to include const char *ssid and *password for your wifi

WiFiServer server(80);

#define DATA_PIN 4
#define SERIAL_SPEED 115200

// How many LEDs in your strip?
#define NUM_LEDS 35 // Total number of LEDs

// How are the LEDs distributed?
#define SegmentSize 5      // How many LEDs in each "Magnetic Constrictor" segment
#define TopLEDcount 15     // LEDs above the "Reaction Chamber"
#define ReactionLEDcount 10 // LEDs inside the "Reaction Chamber"
#define BottomLEDcount 10   // LEDs below the "Reaction Chamber"

// Default Settings
#define DefaultWarpFactor 1   // 1-9
#define DefaultMainHue 160    // 1-255	1=Red 32=Orange 64=Yellow 96=Green 128=Aqua 160=Blue 192=Purple 224=Pink 255=Red
#define DefaultSaturation 255 // 1-255
#define DefaultBrightness 128 // 1-255
#define DefaultPattern 1      // 1-5		1=Standard 2=Breach 3=Rainbow 4=Fade 5=Slow Fade

// Initialize internal variables
#define PulseLength SegmentSize * 2
#define TopChases TopLEDcount / PulseLength + 1 * PulseLength
#define TopLEDtotal TopLEDcount + ReactionLEDcount
#define TopDiff TopLEDcount - BottomLEDcount
#define RateMultiplier 3
byte MainHue = DefaultMainHue;
byte ReactorHue = DefaultMainHue;
byte LastHue = DefaultMainHue;
byte WarpFactor = DefaultWarpFactor;
byte LastWarpFactor = DefaultWarpFactor;
byte Rate = RateMultiplier * WarpFactor;
byte Saturation = DefaultSaturation;
byte Brightness = DefaultBrightness;
byte Pattern = DefaultPattern;
byte Pulse;
boolean Rainbow = false;
boolean Fade = false;
boolean SlowFade = false; // Default Settings

// Serial input variables
const byte numChars = 20;
char receivedChars[numChars];
char tempChars[numChars]; // temporary array for use when parsing

// Parsing variables
byte warp_factor = WarpFactor;
byte hue = MainHue;
byte saturation = Saturation;
byte brightness = Brightness;
byte pattern = Pattern;

bool newData = false;
bool newWifiData = false;

// Define the array of LEDarray
CRGB LEDarray[NUM_LEDS];

void setup()
{
  delay(2000); // 2 second delay for recovery
  Serial.begin(SERIAL_SPEED);

  Serial.println("WiFi start");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  // Start the server
  server.begin();
  Serial.println("Server started");
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(LEDarray, NUM_LEDS);
  FastLED.setCorrection(CRGB(255, 200, 245));
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
  FastLED.setBrightness(Brightness);

  PrintInfo();
}

void loop()
{
  receiveSerialData();
  receiveWifiData();
  if (newData)
  {
    if (!newWifiData)
    {
      strcpy(tempChars, receivedChars); // this is necessary because strtok() in parseData() replaces the commas with \0
      parseData();
    }
    updateSettings();
    //newData = false;
    //newWifiData = false;
  }

  if (Pattern == 1)
  {
    standard();
  }
  else if (Pattern == 2)
  {
    breach();
  }
  else if (Pattern == 3)
  {
    rainbow();
  }
  else if (Pattern == 4)
  {
    fade();
  }
  else if (Pattern == 5)
  {
    slowFade();
  }
  else
  {
    standard();
  }
}

void standard()
{
  ReactorHue = MainHue;

  chase();
}

void breach()
{
  byte breach_diff = 255 - LastHue;
  byte transition_hue = LastHue + (breach_diff / 2);
  if (ReactorHue < 255)
  {
    incrementReactorHue();
  }
  if (ReactorHue > transition_hue && MainHue < 255)
  {
    incrementMainHue();
  }
  if (ReactorHue >= 255 && MainHue >= 255)
  {
    MainHue = LastHue;
    ReactorHue = MainHue + 1;
  }
  Rate = (((ReactorHue - MainHue) / (breach_diff / 9) + 1) * RateMultiplier);
  WarpFactor = Rate / RateMultiplier;
  chase();
}

void rainbow()
{
  Rainbow = true;
  chase();
  Rainbow = false;
}

void fade()
{
  Fade = true;
  chase();
  Fade = false;
}

void slowFade()
{
  SlowFade = true;
  chase();
  SlowFade = false;
}

void incrementHue()
{
  incrementMainHue();
  incrementReactorHue();
}

void incrementReactorHue()
{
  if (MainHue == 255)
  {
    ReactorHue = 1;
  }
  else
  {
    ReactorHue++;
  }
}

void incrementMainHue()
{
  if (MainHue == 255)
  {
    MainHue = 1;
  }
  else
  {
    MainHue++;
  }
}

void chase()
{
  if (Pulse == PulseLength - 1)
  {
    Pulse = 0;
    if (SlowFade == true)
    {
      incrementHue();
    }
  }
  else
  {
    Pulse++;
  }
  if (Fade == true)
  {
    incrementHue();
  }
  // Ramp LED brightness
  for (int value = 32; value < 255; value = value + Rate)
  {
    if (Rainbow == true)
    {
      incrementHue();
    }
    // Set every Nth LED
    for (int chases = 0; chases < TopChases; chases = chases + PulseLength)
    {
      byte Top = Pulse + chases;
      byte Bottom = NUM_LEDS + TopDiff - (Pulse + chases) - 1;
      if (Top < TopLEDtotal)
      {
        LEDarray[Top] = CHSV(MainHue, Saturation, value);
      }
      if (Bottom > TopLEDcount && Bottom < NUM_LEDS)
      {
        LEDarray[Bottom] = CHSV(MainHue, Saturation, value);
      }
    }
    // Keep reaction chamber at full brightness even though we chase the leds right through it
    for (int reaction = 0; reaction < ReactionLEDcount; reaction++)
    {
      LEDarray[TopLEDcount + reaction] = CHSV(ReactorHue, Saturation, 255);
      //LEDarray[TopLEDcount + reaction] = CHSV(0, Saturation, value);
    }
    fadeToBlackBy(LEDarray, NUM_LEDS, (Rate * 0.5)); // Dim all LEDs by Rate/2
    FastLED.show();                                  // Show set LEDs
  }
}

void receiveWifiData()
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client)
  {
    return;
  }

  // Wait until the client sends some data
  Serial.println("Connect");
  if (client.available())
  {

    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.println(request);
    if (request.indexOf("favicon") != -1)
    { // dump favicon requests
      newWifiData = newData = false;
      client.stop();
      return;
    }

    client.flush();

    newWifiData = newData = true;
    // Match the request
    if (request.indexOf("/breach") != -1)
    {
      Serial.println("Breach!");
      pattern = 2;
    }
    else if (request.indexOf("/warp/") != -1)
    {
      String new_warp = request.substring(10, 11);
      warp_factor = new_warp.toInt();
      Serial.println(new_warp);
    }
    else if (request.indexOf("/warp") != -1)
    {
      Serial.println("Warp");
      pattern = 1;
      warp_factor = DefaultWarpFactor;
      WarpFactor = warp_factor;
      LastWarpFactor = warp_factor;
      Rate = RateMultiplier * WarpFactor;
      hue = DefaultMainHue;
      saturation = DefaultSaturation;
      brightness = DefaultBrightness;
      MainHue = hue;
      ReactorHue = hue;
      LastHue = hue;
    }

    if (request.length() == 15)
    {
      newWifiData = newData = false;
    }

    // Return the response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); //  do not forget this one
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<p><a href=\"/\"\">Warp Core</a></p>");
    if (pattern == 1)
    {
      client.print("<p>Warp: ");
      client.print(warp_factor);
      client.println("</p>");
    }
    else
    {
      client.println("<p>Warp Core Breach!</p>");
    }
    client.println("<br />");
    if (pattern == 1)
    {
      client.println("<a href=\"/breach\"\"><button>Breach </button></a>");
    }
    else
    {
      client.println("<a href=\"/warp\"\"><button>Warp </button></a>");
    }
    client.println("<br><br>");
    if (pattern == 1)
    {
      client.println("<a href=\"/warp/1\"\"><button>1</button></a>&nbsp;");
      client.println("<a href=\"/warp/2\"\"><button>2</button></a>&nbsp;");
      client.println("<a href=\"/warp/3\"\"><button>3</button></a>&nbsp;");
      client.println("<a href=\"/warp/4\"\"><button>4</button></a>&nbsp;");
      client.println("<a href=\"/warp/5\"\"><button>5</button></a>&nbsp;");
      client.println("<a href=\"/warp/6\"\"><button>6</button></a>&nbsp;");
      client.println("<a href=\"/warp/7\"\"><button>7</button></a>&nbsp;");
      client.println("<a href=\"/warp/8\"\"><button>8</button></a>&nbsp;");
      client.println("<a href=\"/warp/9\"\"><button>9</button></a><br />");
    }
    client.println("</html>");
    delay(1);
    client.stop();
    Serial.println("Disconnect");
    Serial.println("");
  }
}

void receiveSerialData()
{
  static bool recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char helpMarker = '?';

  char rc;

  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();

    if (rc == helpMarker)
    {
      PrintInfo();
    }
    else if (recvInProgress == true)
    {
      if (rc != endMarker)
      {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars)
        {
          ndx = numChars - 1;
        }
      }
      else if (rc == endMarker)
      {
        receivedChars[ndx] = '\0';
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }
    else if (rc == startMarker)
    {
      recvInProgress = true;
    }
  }
}

void parseData()
{
  char *strtokIndx;                    // this is used by strtok() as an index
  strtokIndx = strtok(tempChars, ","); // get the first part of the string
  warp_factor = atoi(strtokIndx);      // convert this part to an integer
  strtokIndx = strtok(NULL, ",");      // this continues where the previous call left off
  hue = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ",");
  saturation = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ",");
  brightness = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ",");
  pattern = atoi(strtokIndx);
}

void updateSettings()
{
  if (pattern > 0 && pattern < 6 && pattern != Pattern)
  {
    warp_factor = DefaultWarpFactor;
    Rate = RateMultiplier * WarpFactor;
    hue = DefaultMainHue;
    saturation = DefaultSaturation;
    brightness = DefaultBrightness;
    Pattern = pattern;
    Serial.print("Color Pattern Set To ");
    Serial.println(Pattern);
    updateSettings();
  }
  else
  {
    if (warp_factor > 0 && warp_factor < 10 && warp_factor != LastWarpFactor)
    {
      WarpFactor = warp_factor;
      LastWarpFactor = warp_factor;
      Rate = RateMultiplier * WarpFactor;
      Serial.print(F("Warp Factor Set To "));
      Serial.println(warp_factor);
    }
    if (hue > 0 && hue < 256 && hue != LastHue)
    {
      MainHue = hue;
      ReactorHue = hue;
      LastHue = hue;
      Serial.print(F("Color Hue Set To "));
      Serial.println(hue);
    }
    if (saturation > 0 && saturation < 256 && saturation != Saturation)
    {
      Saturation = saturation;
      Serial.print(F("Color Saturation Set To "));
      Serial.println(saturation);
    }
    if (brightness > 0 && brightness < 256)
    {
      FastLED.setBrightness(brightness);
      Brightness = brightness;
      Serial.print(F("Brightness Set To "));
      Serial.println(brightness);
    }
  }
  newData = false;
  newWifiData = newData;
}

void PrintInfo()
{
  Serial.println(F("******** Help ********"));
  Serial.println(F("Input Format - <2, 160, 220, 255>"));
  Serial.println(F("Input Fields - <Warp Factor, Hue, Saturation, Brightness, Pattern>"));
  Serial.println(F("Warp Factor range - 1-9"));
  Serial.println(F("Hue range - 1-255 1=Red 32=Orange 64=Yellow 96=Green 128=Aqua 160=Blue 192=Purple 224=Pink 255=Red"));
  Serial.println(F("Saturation range - 1-255"));
  Serial.println(F("Brightness range - 1-255"));
  Serial.println(F("Pattern - 1-5 1=Standard 2=Breach 3=Rainbow 4=Fade 5=Slow Fade"));
  Serial.println(F(""));
  Serial.println(F("** Current Settings **"));
  Serial.print(F(" <"));
  Serial.print(WarpFactor);
  Serial.print(F(", "));
  Serial.print(MainHue);
  Serial.print(F(", "));
  Serial.print(Saturation);
  Serial.print(F(", "));
  Serial.print(Brightness);
  Serial.print(F(", "));
  Serial.print(Pattern);
  Serial.println(F(">"));
  Serial.println(F("**********************"));
}
