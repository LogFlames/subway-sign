#include <ESP8266WiFi>

#include "credentials.h"

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.print("Connecting to ")
    Serial.println(SSI)
}

void loop() {
    // put your main code here, to run repeatedly:
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
}
