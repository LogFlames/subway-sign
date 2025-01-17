#include <string.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include <PxMatrix.h>
#include <Ticker.h>

#define BUTTON_PIN D0
#define P_LAT D0
#define P_A D1
#define P_B D2
#define P_C D8
#define P_D D6
#define P_OE D4
// #define CLK D5 // Set somewhere within PxMatrix library
// #define R0 D7  // Set somewhere within PxMatrix library

#define UPDATE_INTERVAL 5000

#define DEBUG_SERIAL false

#if DEBUG_SERIAL
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

Ticker display_ticker;
PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);

ESP8266WebServer server(80);
String ssid = "";
String password = "";
String remoteUrl = "";
String configString = "";

Preferences prefs;

String url = "";

unsigned long statusLEDUntil = millis();
unsigned long lastUpdate = millis();

bool APMode = false;

WiFiClient client;

uint16_t myColor = display.color565(255, 150, 30);

HTTPClient http;

void display_updater()
{
  display.display(70);
}

void setup() {
    if (DEBUG_SERIAL) Serial.begin(19200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    prefs.begin("t-skylt");

    loadSettings();

    delay(200);

    if (digitalRead(BUTTON_PIN) == LOW) {
        DEBUG_PRINTLN("Button pressed - starting Access Point");
        startAccessPoint();
        APMode = true;
    } else {
        DEBUG_PRINTLN("Button not pressed - connecting to WiFi");
        connectToWiFi();
        APMode = false;
    }

    pinMode(BUTTON_PIN, OUTPUT);

    display.begin(16);
    display.flushDisplay();
    
    display_ticker.attach_ms_scheduled_accurate(4, display_updater);

    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();

    http.begin(client, url.c_str());
}

void loop() {
    if (APMode) {
        if (statusLEDUntil < millis() - 1000UL) {
            statusLEDUntil = millis() + 1000UL;
        }
    } else {
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("Disconnected, trying to reconnect...");

            waitForWifi();

            DEBUG_PRINTLN();
            DEBUG_PRINT("Connected, IP address: ");
            DEBUG_PRINTLN(WiFi.localIP());
        }

        if (millis() - lastUpdate > UPDATE_INTERVAL) {
            lastUpdate = millis();
            //statusLEDUntil = millis() + 1000UL;

            httpRequest(url);
            //text[0] = 0;
            DEBUG_PRINTF("%s\n", text);
        }
    }

    server.handleClient();
}

void waitForWifi() {
    while (WiFi.status() != WL_CONNECTED) {
        delay(125);
        digitalWrite(LED_BUILTIN, LOW);
        delay(125);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(125);
        digitalWrite(LED_BUILTIN, LOW);
        delay(125);
        digitalWrite(LED_BUILTIN, HIGH);
        DEBUG_PRINT(".");
    }
}

// Function to save settings to EEPROM
void saveSettings(String ssid, String password, String remoteUrl, String configString) {
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.putString("remote_url", remoteUrl);
    prefs.putString("config_string", configString);

    url = remoteUrl + "/text?config=" + configString;
}

// Function to load settings from EEPROM
void loadSettings() {
    ssid = prefs.getString("ssid");
    password = prefs.getString("password");
    remoteUrl = prefs.getString("remote_url");
    configString = prefs.getString("config_string");

    url = remoteUrl + "/text?config=" + configString;
}

// Function to handle root web page
void handleRoot() {
    auto htmlEscape = [](const String &str) -> String {
        String escaped = str;
        escaped.replace("&", "&amp;");
        escaped.replace("<", "&lt;");
        escaped.replace(">", "&gt;");
        escaped.replace("\"", "&quot;");
        escaped.replace("'", "&#39;");
        return escaped;
    };

    String page = "<html><body>"
        "<h1>WiFi Configuration</h1>"
        "<form action='/save' method='POST'>"
        "SSID: <input type='text' name='ssid' value='" + htmlEscape(ssid) + "'><br>"
        "Password: <input type='password' name='password' value='" + htmlEscape(password) + "'><br>"
        "Remote URL: <input type='text' name='url' value='" + htmlEscape(remoteUrl) + "' placeholder='https://tskylt.vkronmar.net'><br>"
        "Config String: <input type='text' name='config' value='" + htmlEscape(configString) + "' placeholder='120;9660;true;28'><br>"
        "<input type='submit' value='Save'>"
        "</form></body></html>";
    server.send(200, "text/html", page);
}

// Function to handle saving the WiFi credentials
void handleSave() {
    ssid = server.arg("ssid");
    password = server.arg("password");
    remoteUrl = server.arg("url");
    configString = server.arg("config");

    saveSettings(ssid, password, remoteUrl, configString);

    String page = "<html><body><h1>Settings Saved!</h1></body></html>";
    server.send(200, "text/html", page);
}

// Setup access point mode
void startAccessPoint() {
    DEBUG_PRINTLN("Opening AP as 'T-Skylten Config'");
    WiFi.softAP("T-Skylten Config");
    IPAddress myIP = WiFi.softAPIP();
    DEBUG_PRINTLN("AP IP address: ");
    DEBUG_PRINTLN(myIP);
}

// Setup station mode
void connectToWiFi() {
    DEBUG_PRINTF("Connecting to: %s\n", ssid.c_str());
    WiFi.hostname("T-Skylten");
    WiFi.begin(ssid.c_str(), password.c_str());

    waitForWifi();

    DEBUG_PRINTLN("WiFi connected");
    DEBUG_PRINT("IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
}

void httpRequest(String url) {
//    HTTPClient http;

    DEBUG_PRINTF("[HTTP] begin... to %s\n", url.c_str());
    //if (http.begin(client, url.c_str())) {
        DEBUG_PRINTLN("[HTTP] GET...");

        int httpCode = http.GET();

        DEBUG_PRINTF("[HTTP] GET... code: %d\n", httpCode);

        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {

                display.clearDisplay();
                display.setTextColor(myColor);
                display.setCursor(2,0);
                display.print(http.getString().c_str());
                //strncpy(text, http.getString().c_str(), TEXT_LENGTH - 1);
                //text[TEXT_LENGTH - 1] = 0;
            } else {
                DEBUG_PRINTLN(http.getString());
                DEBUG_PRINTF("Unexpected HTTP code: %d\n", httpCode);
            }
        } else {
            DEBUG_PRINTF("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        //http.end();
    /*} else {
        DEBUG_PRINTLN("[HTTP] Unable to connect");
    }*/
}
