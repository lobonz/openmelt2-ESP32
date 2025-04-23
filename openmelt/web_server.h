#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

void init_web_server();
void update_web_data(const String &data);
void web_server_task(void *pvParameters);

#endif 
