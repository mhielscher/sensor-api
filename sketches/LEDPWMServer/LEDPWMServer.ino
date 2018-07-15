/*
 * LEDPWMServer.ino Copyright (c) 2018, Matt Hielscher
 * For WeMoS/Lolin D1 mini (ESP8266EX-based)
 *
 * MIT License
 *
 * Copyright (c) 2018 Matt Hielscher
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <ESP8266WiFi.h>
#include <WifiClient.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>

// WiFi and IP settings (examples)
#include "WifiConfig.h"

// Local config
const int ledPin = BUILTIN_LED;
const int pwmPin = D3;

const int httpPort = 80;

const int fadeTime = 1000; // ms to fade to targetBrightness
const int interval = 2; // ms between level updates while fading

// Global Variables
ESP8266WebServer server(httpPort);

uint currentBrightness = 0;
uint targetBrightness = 0;
Ticker fader;

// Utility functions
void fadePWM(int step) {
    if (targetBrightness > currentBrightness) {
        if (step > targetBrightness - currentBrightness)
            currentBrightness = targetBrightness;
        else
            currentBrightness += step;
    }
    else if (targetBrightness < currentBrightness) {
        if (step > currentBrightness - targetBrightness) // reversed because uint
            currentBrightness = targetBrightness;
        else
            currentBrightness -= step;
    }
    currentBrightness = min(1023, currentBrightness);

    analogWrite(pwmPin, currentBrightness);

    if (targetBrightness == currentBrightness)
        fader.detach();
}

// Web route handlers

// <GET />
void handlePWMGet() {
    digitalWrite(onboardLed, 1);

    String response = "{\"brightness\": ";
    response += String(currentBrightness);
    response += ", \"target\": ";
    response += String(targetBrightness);
    response += "}";
    server.send(200, "application/json", response);

    digitalWrite(onboardLed, 0);
}

// <PUT />
void handlePWMPut() {
    digitalWrite(onboardLed, 1);

    if (!server.hasArg("brightness")) {
        server.send(400, "application/json", "{\"error\": \"\\\"brightness\\\" is a required parameter.\"}");
    }
    else if (!isDigit(server.arg("brightness").charAt(0)) || 
             (server.arg("brightness").length() > 1 && server.arg("brightness").toInt() == 0)) {
        // Either this doesn't start with a number, or toInt() couldn't parse it (and returned 0)
        server.send(400, "application/json", "{\"error\": \"Invalid value for parameter \\\"brightness\\\" - must be a number.\"}");
    }
    else if (server.arg("brightness").toInt() < 0 || server.arg("brightness").toInt() > 1023) {
        server.send(400, "application/json", "{\"error\": \"\\\"brightness\\\" must be between 0 and 1023.\"}");
    }
    else {
        // All conditions satisfied?
        fader.detach();
        // min(max()) as a last assurance target is within range
        targetBrightness = min(1023, max(0, (int)server.arg("brightness").toInt()));

        // Calculate the nearest integer step to increase/decrease brightness each interval ms
        // to reach the target in fadeTime ms.
        // If ideal step ends up < 1, fade at a constant rate of 1 step per interval
        int step = max(1, (int)round(interval * ((float)abs(targetBrightness - currentBrightness) / (float)fadeTime)));
        fader.attach_ms(interval, fadePWM, step);
        // Shortcut to return current state; no (meaningful) side effects
        handlePWMGet();
    }

    digitalWrite(onboardLed, 0);
}

// 404 handler
void handleNotFound() {
    digitalWrite(onboardLed, 1);

    String response = "{\"error\": \"URI Not Found\", \"uri\": \"";
    response += server.uri();
    response += "\", \"method\": \"";
    switch (server.method()) {
        case HTTP_GET:
            response += "GET";
            break;
        case HTTP_POST:
            response += "POST";
            break;
        case HTTP_PUT:
            response += "PUT";
            break;
        case HTTP_DELETE:
            response += "DELETE";
            break;
        default:
            response += "[unknown]";
            break;
    }
    response += "\", \"parameters\": {";
    for (uint8_t i=0; i < server.args(); i++) {
        response += "\"" + server.argName(i)+ "\": \"" + server.arg(i) + "\", ";
    }
    response.remove(response.length()-3); // Clean up that last ", "
    response += "}}";

    server.send(404, "application/json", response);

    digitalWrite(onboardLed, 0);
}

void setup() {
    pinMode(onboardLed, OUTPUT);
    pinMode(pwmPin, OUTPUT);
    digitalWrite(onboardLed, 0);
    analogWrite(pwmPin, currentBrightness);

    WiFi.mode(WIFI_STA);
    Wifi.config(static_ip, router_ip, router_ip); // from WifiConfig.h
    Wifi.begin(ssid, password) // also from WifiConfig.h

    while (WiFi.status() != WL_CONNECTED)
        delay(250);

    // Set up routing
    server.on("/", HTTP_GET, handlePWMGet);
    server.on("/", HTTP_PUT, handlePWMPut);
    /* TODO // */ server.on("/", HTTP_POST, handlePWMPut); /* gross hack for using a webform by phone */
    server.onNotFound(handleNotFound);
    server.begin();
}

void loop() {
    server.handleClient();
}
