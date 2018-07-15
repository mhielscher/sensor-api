/*
 * LampRelayServer.ino Copyright (c) 2018, Matt Hielscher
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

// WiFi and IP settings (examples)
#include "WifiConfig.h"

// Local config
const int ledPin = BUILTIN_LED;
const int relayPin = D1;

const int httpPort = 80;

// Global Variables
ESP8266WebServer server(httpPort);

bool relayOn = false;

// Web route handlers

// <GET />
void handleLampGet() {
    digitalWrite(ledPin, 1);

    String response = "{\"state\": \"";
    response += relayOn ? "on" : "off";
    response += "\"}";
    server.send(200, "application/json", response);

    digitalWrite(ledPin, 0);
}

// <PUT />
void handleLampPut() {
    digitalWrite(ledPin, 1);

    if (server.arg("state").equals("on")) {
        relayOn = true;
        server.send(200, "application/json", "{\"state\": \"on\"}");
    }
    else if (server.arg("state").equals("off")) {
        relayOn = false;
        server.send(200, "application/json", "{\"state\": \"off\"}");
    }
    else if (server.arg("state").equals("toggle")) {
        relayOn = !relayOn;
        String response = "{\"state\": \"";
        response += (relayOn ? "on" : "off");
        response += "\"}";
        server.send(200, "application/json", response);
    }
    else {
        String response = "{\"error\": \"Unrecognized argument value.\", ";
        response += "\"state\": \"";
        response += (relayOn ? "on" : "off");
        response += "\"}";
        server.send(400, "application/json", response);
    }
    digitalWrite(relayPin, relayOn);

    digitalWrite(ledPin, 0);
}

// 404 handler
void handleNotFound() {
    digitalWrite(onboardLed, 1);

    // Compose a JSON response with error details
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
    pinMode(ledPin, OUTPUT);
    pinMode(relayPin, OUTPUT);
    digitalWrite(ledPin, 0);
    digitalWrite(relayPin, relayOn);

    WiFi.mode(WIFI_STA);
    Wifi.config(static_ip, router_ip, router_ip); // from WifiConfig.h
    Wifi.begin(ssid, password) // also from WifiConfig.h

    while (WiFi.status() != WL_CONNECTED)
        delay(250);

    // Set up routing
    server.on("/", HTTP_GET, handleLampGet);
    server.on("/", HTTP_PUT, handleLampPut);
    server.onNotFound(handleNotFound);
    server.begin();
}

void loop() {
    server.handleClient();
}
