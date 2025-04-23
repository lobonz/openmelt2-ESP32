#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Arduino.h>

// External declarations
extern WebServer server;
// We don't need the watchdog service in this task
// extern void service_watchdog();

// DNS Server for captive portal
const byte DNS_PORT = 53;
DNSServer dnsServer;

// For storing the latest diagnostic data
String latestData = "";

// Mutex for protecting access to the data
portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

// HTML template for the web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Hammertime Diagnostics</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="1"> <!-- Auto refresh every 1 second -->
  <style>
    body { font-family: Arial; margin: 20px; }
    .card { background-color: #f8f9fa; padding: 20px; margin-bottom: 20px; border-radius: 10px; }
    h2 { color: #343a40; }
    .data { font-family: monospace; white-space: pre-wrap; }
  </style>
</head>
<body>
  <h2>Hammertime Diagnostics</h2>
  <div class="card">
    <div class="data" id="diagnostics">%DIAGNOSTICS%</div>
  </div>
</body>
</html>
)rawliteral";

// Update the data with thread safety
void update_web_data(const String &data) {
  portENTER_CRITICAL(&dataMux);
  latestData = data;
  portEXIT_CRITICAL(&dataMux);
}

// Handle root path
void handleRoot() {
  // No need to reset watchdog here
  String html = index_html;
  portENTER_CRITICAL(&dataMux);
  html.replace("%DIAGNOSTICS%", latestData);
  portEXIT_CRITICAL(&dataMux);
  server.send(200, "text/html", html);
  // No need to reset watchdog here
}

// Handle captive portal requests
void handleCaptivePortal() {
  // Redirect to root
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// Special response for Apple devices
void handleApple() {
  server.send(200, "text/html", "Success");
}

// Task that runs on the second core
void web_server_task(void *pvParameters) {
  Serial.println("*** Web server task starting... ***");
  // No need to reset watchdog here
  
  // WiFi AP is already set up in the main setup function
  // Just use the existing setup
  
  // Setup captive portal DNS server
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("*** DNS Server started for captive portal ***");
  
  // Route for root / web page
  server.on("/", handleRoot);
  
  // Special routes for iOS/macOS captive portal detection
  server.on("/hotspot-detect.html", handleRoot);
  server.on("/library/test/success.html", handleApple);
  server.on("/generate_204", handleRoot);  // Android captive portal detection
  server.on("/connecttest.txt", handleRoot); // Windows captive portal detection
  
  // Catch-all handler for captive portal
  server.onNotFound(handleCaptivePortal);

  // Start server
  server.begin();
  Serial.println("*** HTTP server started ***");
  // No need to reset watchdog here

  // Task loop - continuously handle client requests
  for(;;) {
    // Process DNS requests
    dnsServer.processNextRequest();
    
    // Handle web server clients
    server.handleClient();
    
    // Small delay to prevent task starvation
    delay(5);
    // No need to reset watchdog from this task
  }
}

// Initialize the web server on the second core
void init_web_server() {
  Serial.println("*** Initializing web server on second core... ***");
  // No need to reset watchdog here
  
  // Create task on core 1 (second core)
  xTaskCreatePinnedToCore(
    web_server_task,   // Function to implement the task
    "WebServerTask",   // Name of the task
    8192,              // Stack size in words
    NULL,              // Task input parameter
    1,                 // Priority of the task
    NULL,              // Task handle
    1                  // Core where the task should run (1 = second core)
  );
  
  Serial.println("*** Web server task created ***");
  // No need to reset watchdog here
} 
