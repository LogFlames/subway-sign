#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>

#include "credentials.h"

#define UPDATE_INTERVAL 5000
#define BUFFER_SIZE 256

const char *SL_STATIONS_URL = "http://transport.integration.sl.se/v1/sites?expand=false";
const char *SL_LINES_URL = "http://transport.integration.sl.se/v1/lines";
// DEPARTURES: http://transport.integration.sl.se/v1/sites/{siteId}/departures
const char *SL_DEPARTURES_URL_START = "http://transport.integration.sl.se/v1/sites/";
const char *SL_DEPARTURES_URL_END = "/departures";

ESP8266WebServer server(80);

void setup()
{
    Serial.begin(19200);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.hostname("subway-sign");
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);

    waitForWifi();

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handle_root);

    server.begin();
}

void loop()
{
    // put your main code here, to run repeatedly:
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Disconnected, trying to reconnect...");

        waitForWifi();

        Serial.println();
        Serial.print("Connected, IP address: ");
        Serial.println(WiFi.localIP());
    }

    server.handleClient();
}

void waitForWifi()
{
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(125);
        digitalWrite(LED_BUILTIN, LOW);
        delay(125);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(125);
        digitalWrite(LED_BUILTIN, LOW);
        delay(125);
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.print(".");
    }
}

void handle_root()
{
    server.send(200, "text/html", httpGETRequest(String(SL_DEPARTURES_URL_START) + "9662" + SL_DEPARTURES_URL_END));
}

void skip_whitespace(WiFiClient* client) {
    while (client->available()) {
        char ch = client->read();
        if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\r') {
            client->write(ch); // Write back the non-whitespace character
            break;
        }
    }
}

// Function to read a string value (assuming it's properly quoted)
String read_string(WiFiClient* client) {
    String result;
    char ch;
    while (client->available()) {
        ch = client->read();
        if (ch == '"') {
            break;
        }
        result += ch;
    }
    return result;
}

// Function to check if the current field matches the target
bool field_matches(const String& field_name, const String& target) {
    return field_name.equals(target);
}

// Function to process a nested object
void process_object(WiFiClient* client, const String& field_name) {
    while (client->available()) {
        char ch = client->read();
        if (ch == '"') {
            String value = read_string(client);
            if (field_matches(field_name, "destination") ||
                field_matches(field_name, "display")) {
                Serial.print(field_name);
                Serial.print(": ");
                Serial.println(value);
            } else if (field_matches(field_name, "id")) {
                Serial.print("line id: ");
                Serial.println(value);
            }
        } else if (ch == '{') {
            process_object(client, String());
        } else if (ch == '}') {
            break;
        } else if (ch == ':') {
            String key = read_string(client);
            if (field_matches(key, "line")) {
                client->read(); // consume the opening '{'
                process_object(client, String());
            } else {
                process_object(client, key);
            }
        } else if (ch == ',') {
            continue;
        } else if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\r') {
            continue;
        }
    }
}

void process_stream(WiFiClient* client) {
    while (client->available()) {
        char ch = client->read();
        String field_name;
        bool in_string = false;
        bool is_key = true;

        if (ch == '"') {
            if (in_string) {
                if (is_key) {
                    field_name = field_name;
                } else {
                    if (field_matches(field_name, "destination") ||
                        field_matches(field_name, "display")) {
                        Serial.print(field_name);
                        Serial.print(": ");
                        Serial.println(field_name);
                    }
                }
                in_string = false;
                is_key = !is_key;
            } else {
                in_string = true;
                field_name = read_string(client);
            }
        } else if (ch == '{') {
            process_object(client, String());
        } else if (ch == '}') {
            is_key = true;
        } else if (ch == ':') {
            is_key = false;
        } else if (ch == ',') {
            is_key = true;
        } else if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\r') {
            continue;
        }
    }
}

String httpGETRequest(String url)
{
    HTTPClient http;
    WiFiClient client;

    String payload = "";

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, url.c_str()))
    {
        Serial.print("[HTTP] GET...\n");

        int httpCode = http.GET();

        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                WiFiClient *stream = http.getStreamPtr();
                while (stream->available())
                {
                    char c = stream->read();
                    Serial.print(c);
                }

                // process_stream(stream);
            }
            else
            {
                Serial.printf("Unexpected HTTP code: %d\n", httpCode);
            }
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    else
    {
        Serial.printf("[HTTP] Unable to connect\n");
    }

    return payload;
}