#include <Arduino.h>

#include <TFT_eSPI.h> 
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <ESP32Ping.h>
#include <Button2.h>
//#include <esp_adc_cal.h>
#include <time.h>
#include "CircularBuffer.h"


#include "config.h"

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define ADC_EN          14
#define ADC_PIN         34
#define BUTTON_1        35
#define BUTTON_2        0

#define BRIGHTNESS_MIN  0

#ifndef CONFIG_EXTERNAL
const char *ssid = "your access point";
const char *password = "password";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; //germany offset versus GMT
const int daylightOffset_sec = 3600;  // called "sommerzeit" in germany.
const IPAddress googledns_ip(8,8,8,8); //google dns
#endif


const uint8_t ledChannel = 1;
uint16_t lastState = TFT_DARKGREY;
uint8_t currentBrightness = 255;
uint16_t dimmTimeout = 3000; // time in ms 10000= 10 secs

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

struct event
{
    uint16_t state;
    struct tm timeinfo;
};

typedef struct event event_t;

// the type of the record is unsigned long: we intend to store milliseconds
// the buffer can contain up to 10 records
// the buffer will use a byte for its index to reduce memory footprint
CircularBuffer<event_t, 10> events;

void autodimm(bool reset = false)
{
    static uint16_t time_of_dimReset = millis();
    if (reset)
    {
        time_of_dimReset = millis();
        currentBrightness = 255;
        ledcWrite(ledChannel, currentBrightness);
        return;
    }

    if ((time_of_dimReset + dimmTimeout) < millis())
    {
        if (currentBrightness > BRIGHTNESS_MIN)
        {
            currentBrightness = max(currentBrightness - 2, BRIGHTNESS_MIN);
            ledcWrite(ledChannel, currentBrightness); // 0-15, 0-255 (with 8 bit resolution)
        }
        else
        {
            //
        }
    }
}

void showCurrentEvent()
{
    autodimm(true);
    event_t event = events.last();
    tft.fillScreen(event.state);
    tft.setTextColor(TFT_BLACK, event.state);
    tft.setRotation(3);
    tft.setTextDatum(MC_DATUM);

    if (!getLocalTime(&event.timeinfo))
    {
        return;
    }
    char buff[255];
    strftime(buff, 255, "%A, %B %d %Y %H:%M:%S", &event.timeinfo);
    tft.drawString("Last Update:", tft.width() / 2, tft.height() / 2 - tft.fontHeight() - 2);
    tft.drawString(buff, tft.width() / 2, tft.height() / 2);
}

void listLastEvents()
{
    autodimm(true);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setCursor(0, 0);
    tft.setTextSize(1);
    char buff[255];
    for (int i = 0; i < events.size(); ++i)
    {
        event_t event = events[i];
        strftime(buff, 255, "%A, %B %d %Y %H:%M:%S", &event.timeinfo);
        tft.setTextColor(TFT_BLACK, event.state);
        tft.println(buff);
    }
}

void recordEventState(uint16_t newState)
{
    if (lastState != newState)
    {
        lastState = newState;
        autodimm(true);

        event_t event;
        event.state = newState;
        getLocalTime(&event.timeinfo);
        events.push(event);
        showCurrentEvent();
    }
}

wl_status_t wifi_connect()
{
    recordEventState(TFT_ORANGE);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(false, true);
    WiFi.begin(ssid, password);
    
    int8_t retries = 2;
    while (WiFi.status() != WL_CONNECTED && retries > 0)
    {
        Serial.println("Connecting to WiFi..");
        Serial.print("ssid: ");
        Serial.println(ssid);
        Serial.print("password: ");
        Serial.println(password);
        delay(2500);
        retries--;
    }
    return WiFi.status();
}

void wifi_scan()
{
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();

    int16_t n = WiFi.scanNetworks();
    

    tft.fillScreen(TFT_BLACK);
    if (n == 0) {
        tft.drawString("no networks found", tft.width() / 2, tft.height() / 2);
    } else {
        tft.setTextDatum(TL_DATUM);
        tft.setCursor(0, 0);
        Serial.printf("Found %d net\n", n);
        char buff[255];
        for (int i = 0; i < n; ++i) {
            sprintf(buff,
                    "[%d]:%s(%d)",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i));
            tft.println(buff);
        }
    }
    WiFi.mode(WIFI_OFF);
}

void button_init()
{
    btn1.setPressedHandler([](Button2 &b)
                           { showCurrentEvent(); });

    btn2.setPressedHandler([](Button2 &b)
                           { listLastEvents(); });
}

void ping()
{
    if(Ping.ping(googledns_ip,1))
    {
        recordEventState(TFT_GREEN);
        autodimm();
        return;
    }
    recordEventState(TFT_RED);
}


void button_loop()
{
    btn1.loop();
    btn2.loop();
}

void ping_loop()
{
    static uint32_t lastcall = 0;

    
    if(WiFi.status() == WL_CONNECTED )
    {
        if(millis() > lastcall + 500)
        {
            ping();
            lastcall = millis();
        }
    }
    else
    {
        recordEventState(TFT_ORANGE);
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        WiFi.reconnect();
    }
}

void tft_init()
{
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setRotation(0);

    if (TFT_BL > 0)
    {                                           // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        // set PWM for backlight brightness
        ledcSetup(ledChannel, 5000, 8);    // 0-15, 5000, 8
        ledcAttachPin(TFT_BL, ledChannel); // TFT_BL, 0 - 15
        autodimm(true);
    }
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    Serial.print("Day of week: ");
    Serial.println(&timeinfo, "%A");
    Serial.print("Month: ");
    Serial.println(&timeinfo, "%B");
    Serial.print("Day of Month: ");
    Serial.println(&timeinfo, "%d");
    Serial.print("Year: ");
    Serial.println(&timeinfo, "%Y");
    Serial.print("Hour: ");
    Serial.println(&timeinfo, "%H");
    Serial.print("Hour (12 hour format): ");
    Serial.println(&timeinfo, "%I");
    Serial.print("Minute: ");
    Serial.println(&timeinfo, "%M");
    Serial.print("Second: ");
    Serial.println(&timeinfo, "%S");

    Serial.println("Time variables");
    char timeHour[3];
    strftime(timeHour, 3, "%H", &timeinfo);
    Serial.println(timeHour);
    char timeWeekDay[10];
    strftime(timeWeekDay, 10, "%A", &timeinfo);
    Serial.println(timeWeekDay);
    Serial.println();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Start");

    tft_init();
    button_init();
  
    if( wifi_connect() == WL_CONNECTED) 
    {
        Serial.println("WiFi connected");
        // Init and get the time
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        //printLocalTime();
    }
    else
    {
        Serial.println("WiFi NOT connected - restarting device!");
        ESP.restart();
    }
}

void loop()
{
    button_loop();
    ping_loop();
}
