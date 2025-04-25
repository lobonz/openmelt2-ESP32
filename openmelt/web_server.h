#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

void init_web_server();
void update_web_data(const String &telemetry, const String &logs);
void web_server_task(void *pvParameters);
String parseTelemetryToJSON(const String &telemetryData);
void handleEEPROM();
void handleNotFound();

#endif 
