#include "web_server.h"
#include "debug_handler.h"
#include "config_storage.h"
#include "melty_config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>

// External declarations
extern WebServer server;
extern const char* ssid;      // WiFi SSID from openmelt.ino
extern const char* password;  // WiFi password from openmelt.ino
// We don't need the watchdog service in this task
// extern void service_watchdog();

// Configuration
#define WEB_SERVER_TASK_STACK_SIZE 16384
#define UPDATE_INTERVAL_MS 250

// Buffers for data
char webTelemetryBuffer[1024] = "";
char webLogBuffer[2048] = "";

// Mutex for protecting access to the data
portMUX_TYPE webDataMux = portMUX_INITIALIZER_UNLOCKED;

// Web server
WebServer webServer(80);

// Flag indicating if the server is running
volatile bool server_running = false;

// HTML template with JavaScript for a nice UI
const char* htmlTemplate = R"(
<!DOCTYPE html>
<html>
<head>
  <title>OpenMelt Telemetry</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background-color: #f0f0f0;
      color: #333;
    }
    h1 {
      color: #222;
      margin-bottom: 20px;
    }
    #hud {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 10px;
      margin-bottom: 20px;
    }
    .metric {
      background-color: #fff;
      border-radius: 8px;
      padding: 15px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      text-align: center;
    }
    .metric-title {
      font-size: 14px;
      color: #666;
      margin-bottom: 5px;
    }
    .metric-value {
      font-size: 24px;
      font-weight: bold;
      color: #0066cc;
    }
    .metric-unit {
      font-size: 14px;
      color: #666;
    }
    #logs {
      background-color: #fff;
      border-radius: 8px;
      padding: 15px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      height: 300px;
      overflow-y: auto;
      font-family: monospace;
      white-space: pre-wrap;
      font-size: 14px;
      line-height: 1.4;
    }
    .warning {
      color: #ff9900;
    }
    .error {
      color: #cc0000;
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
    }
    .button {
      background-color: #0066cc;
      color: white;
      border: none;
      padding: 8px 16px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 14px;
    }
    .button:hover {
      background-color: #0055aa;
    }
    .metric-container {
      display: flex;
      justify-content: space-around;
      margin-top: 10px;
    }
    .metric-child {
      text-align: center;
      flex: 1;
      padding: 0 5px;
    }
    .metric-child[id="child-maxRpm"] .metric-value,
    .metric-child[id="child-maxGForce"] .metric-value {
      color: #e74c3c; /* Red color for max values */
    }
    .max-value {
      color: #e74c3c; /* Red color for max values */
    }
    .metric-value-container {
      display: flex;
      justify-content: center;
      align-items: center;
      font-size: 24px;
      font-weight: bold;
    }
    .metric-separator {
      margin: 0 2px;
      color: #666;
    }
    .metric-subtitle {
      font-size: 12px;
      color: #666;
      margin-bottom: 2px;
    }
    .reset-button {
      background-color: #ff6b6b;
      margin-left: 10px;
    }
    /* Tab styles */
    .tabs {
      display: flex;
      margin-bottom: 20px;
      border-bottom: 1px solid #ddd;
    }
    .tab {
      padding: 10px 20px;
      cursor: pointer;
      background-color: #f0f0f0;
      border: 1px solid #ddd;
      border-bottom: none;
      border-radius: 4px 4px 0 0;
      margin-right: 5px;
    }
    .tab.active {
      background-color: #fff;
      border-bottom: 1px solid #fff;
      margin-bottom: -1px;
      font-weight: bold;
    }
    .tab-content {
      display: none;
    }
    .tab-content.active {
      display: block;
    }
    /* EEPROM table styles */
    .eeprom-table {
      width: 100%;
      border-collapse: collapse;
      margin-bottom: 20px;
      background-color: #fff;
      border-radius: 8px;
      overflow: hidden;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .eeprom-table th, .eeprom-table td {
      border: 1px solid #ddd;
      padding: 12px 15px;
      text-align: left;
    }
    .eeprom-table th {
      background-color: #f8f9fa;
      color: #333;
    }
    .eeprom-table tr:nth-child(even) {
      background-color: #f2f2f2;
    }
    /* Graph styles */
    .graphs-container {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
      gap: 15px;
      margin-bottom: 20px;
    }
    .graph-card {
      background-color: #fff;
      border-radius: 8px;
      padding: 15px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .graph-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 10px;
    }
    .graph-title {
      font-size: 16px;
      font-weight: bold;
      margin: 0;
    }
    .graph-controls {
      display: flex;
      gap: 10px;
    }
    .graph-controls button {
      padding: 4px 8px;
      background-color: #f0f0f0;
      border: 1px solid #ddd;
      border-radius: 4px;
      cursor: pointer;
      font-size: 12px;
    }
    .graph-controls button:hover {
      background-color: #e0e0e0;
    }
    .graph-controls button.active {
      background-color: #0066cc;
      color: white;
      border-color: #0055aa;
    }
  </style>
</head>
<body>
  <header>
    <h1>OpenMelt Telemetry</h1>
    <div>
      <button class="button" id="clearLogs">Clear Logs</button>
      <button class="button reset-button" id="resetMax">Reset Max Values</button>
      <button class="button" id="resetGraphs">Reset Graphs</button>
    </div>
  </header>
  
  <div class="tabs">
    <div class="tab active" data-tab="telemetry">Telemetry</div>
    <div class="tab" data-tab="graphs">Graphs</div>
    <div class="tab" data-tab="eeprom">EEPROM Settings</div>
  </div>
  
  <div id="telemetry-tab" class="tab-content active">
    <div id="hud"></div>
    <h2>System Logs</h2>
    <div id="logs"></div>
  </div>
  
  <div id="graphs-tab" class="tab-content">
    <h2>Real-time Telemetry Graphs</h2>
    <div class="graphs-container">
      <div class="graph-card">
        <div class="graph-header">
          <h3 class="graph-title">RPM</h3>
          <div class="graph-controls">
            <button id="rpmPlot-autoScroll" class="active">Auto-scroll</button>
            <button id="rpmPlot-clear">Clear</button>
          </div>
        </div>
        <canvas id="rpmPlot"></canvas>
      </div>
      <div class="graph-card">
        <div class="graph-header">
          <h3 class="graph-title">G-Force</h3>
          <div class="graph-controls">
            <button id="gForcePlot-autoScroll" class="active">Auto-scroll</button>
            <button id="gForcePlot-clear">Clear</button>
          </div>
        </div>
        <canvas id="gForcePlot"></canvas>
      </div>
      <div class="graph-card">
        <div class="graph-header">
          <h3 class="graph-title">Motor Throttle (Motor 1 & 2)</h3>
          <div class="graph-controls">
            <button id="throttlePlot-autoScroll" class="active">Auto-scroll</button>
            <button id="throttlePlot-clear">Clear</button>
          </div>
        </div>
        <canvas id="throttlePlot"></canvas>
      </div>
      <div class="graph-card">
        <div class="graph-header">
          <h3 class="graph-title">Raw Acceleration</h3>
          <div class="graph-controls">
            <button id="accelPlot-autoScroll" class="active">Auto-scroll</button>
            <button id="accelPlot-clear">Clear</button>
          </div>
        </div>
        <canvas id="accelPlot"></canvas>
      </div>
      <div class="graph-card">
        <div class="graph-header">
          <h3 class="graph-title">RC Signal (Throttle & Steering)</h3>
          <div class="graph-controls">
            <button id="rcSignalPlot-autoScroll" class="active">Auto-scroll</button>
            <button id="rcSignalPlot-clear">Clear</button>
          </div>
        </div>
        <canvas id="rcSignalPlot"></canvas>
      </div>
      <div class="graph-card">
        <div class="graph-header">
          <h3 class="graph-title">Battery Voltage</h3>
          <div class="graph-controls">
            <button id="batteryPlot-autoScroll" class="active">Auto-scroll</button>
            <button id="batteryPlot-clear">Clear</button>
          </div>
        </div>
        <canvas id="batteryPlot"></canvas>
      </div>
    </div>
  </div>
  
  <div id="eeprom-tab" class="tab-content">
    <h2>EEPROM Settings</h2>
    <table class="eeprom-table">
      <thead>
        <tr>
          <th>Setting</th>
          <th>Current Value</th>
          <th>Default Value</th>
          <th>Description</th>
        </tr>
      </thead>
      <tbody id="eeprom-settings">
        <tr>
          <td colspan="4">Loading EEPROM settings...</td>
        </tr>
      </tbody>
    </table>
    <button class="button" id="refreshEEPROM">Refresh EEPROM</button>
  </div>
  
  <script src="/TinyLinePlot.js"></script>
  <script>
    // Create HUD elements for the key metrics
    const hudMetrics = [
      { id: 'rpmContainer', title: 'RPM', isContainer: true, children: [
        { id: 'rpm', title: 'Current', unit: 'rpm' },
        { id: 'maxRpm', title: 'Max', unit: 'rpm' }
      ]},
      { id: 'gForceContainer', title: 'G-Force', isContainer: true, children: [
        { id: 'gForce', title: 'Current', unit: 'g' },
        { id: 'maxGForce', title: 'Max', unit: 'g' },
        { id: 'accelUsed', title: 'Used', unit: 'g' }
      ]},
      { id: 'throttleContainer', title: 'Motor Throttle', isContainer: true, children: [
        { id: 'motor1Throttle', title: 'Motor 1', unit: '%', hasMax: true, maxId: 'maxMotor1Throttle' },
        { id: 'motor2Throttle', title: 'Motor 2', unit: '%', hasMax: true, maxId: 'maxMotor2Throttle' }
      ]}
    ];
    
    const hudElement = document.getElementById('hud');
    
    // Create metric elements
    hudMetrics.forEach(metric => {
      const metricElement = document.createElement('div');
      metricElement.className = 'metric';
      
      if (metric.isContainer) {
        // Create container with title
        metricElement.innerHTML = `<div class="metric-title">${metric.title}</div>`;
        
        // Create container for child metrics
        const childContainer = document.createElement('div');
        childContainer.className = 'metric-container';
        
        // Add child metrics
        metric.children.forEach(child => {
          const childElement = document.createElement('div');
          childElement.className = 'metric-child';
          childElement.id = `child-${child.id}`;
          
          if (child.hasMax) {
            childElement.innerHTML = `
              <div class="metric-subtitle">${child.title}</div>
              <div class="metric-value-container">
                <span id="${child.id}" class="metric-value">0</span>
                <span class="metric-separator">/</span>
                <span id="${child.maxId}" class="metric-value max-value">0</span>
              </div>
              <div class="metric-unit">${child.unit}</div>
            `;
          } else {
            childElement.innerHTML = `
              <div class="metric-subtitle">${child.title}</div>
              <div class="metric-value" id="${child.id}">0</div>
              <div class="metric-unit">${child.unit}</div>
            `;
          }
          
          childContainer.appendChild(childElement);
        });
        
        metricElement.appendChild(childContainer);
      } else {
        // Standard metric
        metricElement.innerHTML = `
          <div class="metric-title">${metric.title}</div>
          <div class="metric-value" id="${metric.id}">0</div>
          <div class="metric-unit">${metric.unit}</div>
        `;
      }
      
      hudElement.appendChild(metricElement);
    });
    
    // Track max values
    let maxGForce = 0;
    let maxRpm = 0;
    let maxMotor1Throttle = 0;
    let maxMotor2Throttle = 0;
    
    // Logs container
    const logsElement = document.getElementById('logs');
    
    // Initialize graph plots
    let rpmPlot, gForcePlot, throttlePlot, accelPlot, rcSignalPlot, batteryPlot;
    let timeCounter = 0;
    
    function initGraphs() {
      // Common graph options
      const graphOptions = {
        width: 380,
        height: 200,
        padding: 30,
        showGrid: true,
        lineWidth: 2,
        retainData: true,
        streaming: true,
        viewWindowSize: 60,
        autoScrollWithNewData: true
      };
      
      // RPM Plot
      rpmPlot = new TinyLinePlot('rpmPlot', {
        ...graphOptions,
        xLabel: 'Time (s)',
        yLabel: 'RPM',
        title: 'Rotation Speed'
      });
      rpmPlot.addDataset([], 'RPM').draw();
      
      // G-Force Plot
      gForcePlot = new TinyLinePlot('gForcePlot', {
        ...graphOptions,
        xLabel: 'Time (s)',
        yLabel: 'g',
        title: 'G-Force'
      });
      gForcePlot.addDataset([], 'Raw G').addDataset([], 'Used G').draw();
      
      // Motor Throttle Plot
      throttlePlot = new TinyLinePlot('throttlePlot', {
        ...graphOptions,
        xLabel: 'Time (s)',
        yLabel: '%',
        title: 'Motor Throttle (Motor 1 & 2)'
      });
      throttlePlot.addDataset([], 'Motor 1').addDataset([], 'Motor 2').draw();
      
      // Raw Acceleration Plot
      accelPlot = new TinyLinePlot('accelPlot', {
        ...graphOptions,
        xLabel: 'Time (s)',
        yLabel: 'g',
        title: 'Acceleration XYZ'
      });
      accelPlot.addDataset([], 'X').addDataset([], 'Y').addDataset([], 'Z').draw();
      
      // RC Signal Plot
      rcSignalPlot = new TinyLinePlot('rcSignalPlot', {
        ...graphOptions,
        xLabel: 'Time (s)',
        yLabel: 'Value',
        title: 'RC Signal (Throttle & Steering)'
      });
      rcSignalPlot.addDataset([], 'Throttle').addDataset([], 'Steering').draw();
      
      // Battery Voltage Plot
      batteryPlot = new TinyLinePlot('batteryPlot', {
        ...graphOptions,
        xLabel: 'Time (s)',
        yLabel: 'Volts',
        title: 'Battery Voltage'
      });
      batteryPlot.addDataset([], 'Voltage').draw();
      
      // Setup graph control buttons
      setupGraphControls('rpmPlot', rpmPlot);
      setupGraphControls('gForcePlot', gForcePlot);
      setupGraphControls('throttlePlot', throttlePlot);
      setupGraphControls('accelPlot', accelPlot);
      setupGraphControls('rcSignalPlot', rcSignalPlot);
      setupGraphControls('batteryPlot', batteryPlot);
    }
    
    function setupGraphControls(plotId, plot) {
      document.getElementById(`${plotId}-autoScroll`).addEventListener('click', function() {
        plot.toggleAutoScroll();
        this.classList.toggle('active');
        plot.draw();
      });
      
      document.getElementById(`${plotId}-clear`).addEventListener('click', function() {
        plot.clear();
        
        // Re-add empty datasets based on the plot type
        if (plotId === 'rpmPlot') {
          plot.addDataset([], 'RPM');
        } else if (plotId === 'gForcePlot') {
          plot.addDataset([], 'Raw G').addDataset([], 'Used G');
        } else if (plotId === 'throttlePlot') {
          plot.addDataset([], 'Motor 1').addDataset([], 'Motor 2');
        } else if (plotId === 'accelPlot') {
          plot.addDataset([], 'X').addDataset([], 'Y').addDataset([], 'Z');
        } else if (plotId === 'rcSignalPlot') {
          plot.addDataset([], 'Throttle').addDataset([], 'Steering');
        } else if (plotId === 'batteryPlot') {
          plot.addDataset([], 'Voltage');
        }
        
        plot.draw();
      });
    }
    
    // Function to update the HUD with telemetry data
    function updateHUD(data) {
      // Update regular metrics
      Object.keys(data).forEach(key => {
        const element = document.getElementById(key);
        if (element && data[key] !== undefined) {
          element.textContent = data[key];
          
          // Track max values
          if (key === 'gForce' && parseFloat(data[key]) > maxGForce) {
            maxGForce = parseFloat(data[key]);
            document.getElementById('maxGForce').textContent = maxGForce.toFixed(2);
          }
          
          if (key === 'rpm' && parseInt(data[key]) > maxRpm) {
            maxRpm = parseInt(data[key]);
            document.getElementById('maxRpm').textContent = maxRpm;
          }
          
          if (key === 'motor1Throttle' && parseInt(data[key]) > maxMotor1Throttle) {
            maxMotor1Throttle = parseInt(data[key]);
            document.getElementById('maxMotor1Throttle').textContent = maxMotor1Throttle;
          }
          
          if (key === 'motor2Throttle' && parseInt(data[key]) > maxMotor2Throttle) {
            maxMotor2Throttle = parseInt(data[key]);
            document.getElementById('maxMotor2Throttle').textContent = maxMotor2Throttle;
          }
        }
      });
    }
    
    // Function to update the graphs with new telemetry data
    function updateGraphs(data) {
      timeCounter += 0.5;
      
      // Update RPM Plot
      if (rpmPlot && data.rpm !== undefined) {
        rpmPlot.addPoint(0, { x: timeCounter, y: parseInt(data.rpm) }).draw();
      }
      
      // Update G-Force Plot
      if (gForcePlot) {
        if (data.gForce !== undefined) {
          gForcePlot.addPoint(0, { x: timeCounter, y: parseFloat(data.gForce) });
        }
        if (data.accelUsed !== undefined) {
          gForcePlot.addPoint(1, { x: timeCounter, y: parseFloat(data.accelUsed) });
        }
        gForcePlot.draw();
      }
      
      // Update Motor Throttle Plot
      if (throttlePlot) {
        if (data.motor1Throttle !== undefined) {
          throttlePlot.addPoint(0, { x: timeCounter, y: parseInt(data.motor1Throttle) });
        }
        if (data.motor2Throttle !== undefined) {
          throttlePlot.addPoint(1, { x: timeCounter, y: parseInt(data.motor2Throttle) });
        }
        throttlePlot.draw();
      }
      
      // Update Raw Acceleration Plot (use JSON data)
      if (accelPlot) {
        if (data.accelX !== undefined) {
          accelPlot.addPoint(0, { x: timeCounter, y: parseFloat(data.accelX) });
        }
        if (data.accelY !== undefined) {
          accelPlot.addPoint(1, { x: timeCounter, y: parseFloat(data.accelY) });
        }
        if (data.accelZ !== undefined) {
          accelPlot.addPoint(2, { x: timeCounter, y: parseFloat(data.accelZ) });
        }
        accelPlot.draw();
      }
      
      // Update RC Signal Plot
      if (rcSignalPlot) {
        // Use data from JSON instead of parsing raw logs
        if (data.rcThrottle !== undefined) {
          rcSignalPlot.addPoint(0, { x: timeCounter, y: parseInt(data.rcThrottle) });
        }
        
        if (data.rcSteering !== undefined) {
          rcSignalPlot.addPoint(1, { x: timeCounter, y: parseInt(data.rcSteering) });
        }
        
        rcSignalPlot.draw();
      }
      
      // Update Battery Voltage Plot
      if (batteryPlot && data.battery !== undefined) {
        batteryPlot.addPoint(0, { x: timeCounter, y: parseFloat(data.battery) }).draw();
      }
    }
    
    // Function to update the logs
    function updateLogs(logs) {
      logsElement.innerHTML = logs
        .replace(/⚠️/g, '<span class="warning">⚠️</span>')
        .replace(/🛑/g, '<span class="error">🛑</span>');
      
      // Auto-scroll to bottom
      logsElement.scrollTop = logsElement.scrollHeight;
    }
    
    // Function to update EEPROM settings table
    function updateEEPROMSettings(data) {
      const tableBody = document.getElementById('eeprom-settings');
      tableBody.innerHTML = '';
      
      // Add each setting to the table
      Object.keys(data).forEach(key => {
        const row = document.createElement('tr');
        const setting = data[key];
        
        row.innerHTML = `
          <td>${setting.name}</td>
          <td>${setting.value}${setting.unit || ''}</td>
          <td>${setting.default}${setting.unit || ''}</td>
          <td>${setting.description}</td>
        `;
        
        tableBody.appendChild(row);
      });
    }
    
    // Function to fetch data from the server
    function fetchData() {
      // Fetch telemetry data
      fetch('/telemetry')
        .then(response => response.json())
        .then(data => {
          updateHUD(data);
          updateGraphs(data);
        })
        .catch(error => {
          console.error('Error fetching telemetry:', error);
        });
      
      // Fetch logs separately just for display
      fetch('/logs')
        .then(response => response.text())
        .then(logs => {
          updateLogs(logs);
        })
        .catch(error => {
          console.error('Error fetching logs:', error);
        });
    }
    
    // Function to fetch EEPROM settings
    function fetchEEPROMSettings() {
      fetch('/eeprom')
        .then(response => response.json())
        .then(data => {
          updateEEPROMSettings(data);
        })
        .catch(error => {
          console.error('Error fetching EEPROM settings:', error);
        });
    }
    
    // Tab functionality
    const tabs = document.querySelectorAll('.tab');
    tabs.forEach(tab => {
      tab.addEventListener('click', () => {
        // Remove active class from all tabs and content
        document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
        
        // Add active class to clicked tab
        tab.classList.add('active');
        
        // Show corresponding content
        const tabName = tab.getAttribute('data-tab');
        document.getElementById(`${tabName}-tab`).classList.add('active');
        
        // Load EEPROM data when switching to EEPROM tab
        if (tabName === 'eeprom') {
          fetchEEPROMSettings();
        }
      });
    });
    
    // Clear logs button
    document.getElementById('clearLogs').addEventListener('click', () => {
      fetch('/clear', { method: 'POST' })
        .then(() => {
          logsElement.innerHTML = '';
        })
        .catch(error => {
          console.error('Error clearing logs:', error);
        });
    });
    
    // Reset max values button
    document.getElementById('resetMax').addEventListener('click', () => {
      maxGForce = 0;
      maxRpm = 0;
      maxMotor1Throttle = 0;
      maxMotor2Throttle = 0;
      document.getElementById('maxGForce').textContent = '0';
      document.getElementById('maxRpm').textContent = '0';
      document.getElementById('maxMotor1Throttle').textContent = '0';
      document.getElementById('maxMotor2Throttle').textContent = '0';
    });
    
    // Reset graphs button
    document.getElementById('resetGraphs').addEventListener('click', () => {
      timeCounter = 0;
      if (rpmPlot) rpmPlot.clear().addDataset([], 'RPM').draw();
      if (gForcePlot) gForcePlot.clear().addDataset([], 'Raw G').addDataset([], 'Used G').draw();
      if (throttlePlot) throttlePlot.clear().addDataset([], 'Motor 1').addDataset([], 'Motor 2').draw();
      if (accelPlot) accelPlot.clear().addDataset([], 'X').addDataset([], 'Y').addDataset([], 'Z').draw();
      if (rcSignalPlot) rcSignalPlot.clear().addDataset([], 'Throttle').addDataset([], 'Steering').draw();
      if (batteryPlot) batteryPlot.clear().addDataset([], 'Voltage').draw();
    });
    
    // Refresh EEPROM button
    document.getElementById('refreshEEPROM').addEventListener('click', fetchEEPROMSettings);
    
    // Initialize graphs
    initGraphs();
    
    // Initial fetch
    fetchData();
    
    // Set up interval to fetch data periodically
    setInterval(fetchData, 500);
  </script>
</body>
</html>
)";

// Handler function for root path
void handleRoot() {
  webServer.send(200, "text/html", htmlTemplate);
}

// Handler for telemetry data as JSON
void handleTelemetry() {
  String telemetryData;
  
  // Safely get telemetry data with mutex protection
  portENTER_CRITICAL(&webDataMux);
  telemetryData = String(webTelemetryBuffer);
  portEXIT_CRITICAL(&webDataMux);
  
  // Parse telemetry data into JSON and send response
  String jsonData = parseTelemetryToJSON(telemetryData);
  webServer.send(200, "application/json", jsonData);
}

// Handler for log data as plain text
void handleLogs() {
  String logData;
  
  // Safely get log data with mutex protection
  portENTER_CRITICAL(&webDataMux);
  logData = String(webLogBuffer);
  portEXIT_CRITICAL(&webDataMux);
  
  webServer.send(200, "text/plain", logData);
}

// Handler for clearing logs
void handleClear() {
  clear_debug_data();
  webServer.send(200, "text/plain", "Logs cleared");
}

// Handler for EEPROM settings as JSON
void handleEEPROM() {
  String jsonResult = "{";
  
  // Include LED offset
  int ledOffset = load_heading_led_offset();
  jsonResult += "\"ledOffset\":{";
  jsonResult += "\"name\":\"LED Heading Offset\",";
  jsonResult += "\"value\":" + String(ledOffset) + ",";
  jsonResult += "\"default\":" + String(DEFAULT_LED_OFFSET_PERCENT) + ",";
  jsonResult += "\"unit\":\"%\",";
  jsonResult += "\"description\":\"Position of heading LED (percentage of rotation)\"";
  jsonResult += "},";
  
  // Include accelerometer mount radius
  float accelRadius = load_accel_mount_radius();
  jsonResult += "\"accelRadius\":{";
  jsonResult += "\"name\":\"Accelerometer Mount Radius\",";
  jsonResult += "\"value\":" + String(accelRadius, 2) + ",";
  jsonResult += "\"default\":" + String(DEFAULT_ACCEL_MOUNT_RADIUS_CM, 2) + ",";
  jsonResult += "\"unit\":\"cm\",";
  jsonResult += "\"description\":\"Distance from center of rotation to accelerometer\"";
  jsonResult += "},";
  
  // Include zero G offset
  float zeroGOffset = load_accel_zero_g_offset();
  jsonResult += "\"zeroGOffset\":{";
  jsonResult += "\"name\":\"Zero G Offset\",";
  jsonResult += "\"value\":" + String(zeroGOffset, 3) + ",";
  jsonResult += "\"default\":" + String(DEFAULT_ACCEL_ZERO_G_OFFSET, 3) + ",";
  jsonResult += "\"unit\":\"g\",";
  jsonResult += "\"description\":\"Calibrated zero-point for accelerometer (when stationary)\"";
  jsonResult += "},";
  
  // Add EEPROM sentinel value
  jsonResult += "\"eepromSentinel\":{";
  jsonResult += "\"name\":\"EEPROM Sentinel Value\",";
  jsonResult += "\"value\":" + String(EEPROM_WRITTEN_SENTINEL_VALUE) + ",";
  jsonResult += "\"default\":" + String(EEPROM_WRITTEN_SENTINEL_VALUE) + ",";
  jsonResult += "\"unit\":\"\",";
  jsonResult += "\"description\":\"Changing this value will reset all EEPROM settings to defaults\"";
  jsonResult += "}";
  
  jsonResult += "}";
  webServer.send(200, "application/json", jsonResult);
}

// Handler for serving the TinyLinePlot.js file
void handleTinyLinePlotJS() {
  // Load TinyLinePlot.js content
  static const char* tinyLinePlotJS = R"(/**
 * TinyLinePlot.js - A minimal line plotting library for IoT devices
 * Total size: ~2.5KB minified
 */

class TinyLinePlot {
  constructor(elementId, options = {}) {
    // Get canvas element
    this.canvas = document.getElementById(elementId);
    this.ctx = this.canvas.getContext('2d');
    
    // Default options
    this.options = {
      width: options.width || this.canvas.width || 300,
      height: options.height || this.canvas.height || 200,
      padding: options.padding || 30,
      lineColors: options.lineColors || ['#3366CC', '#DC3912', '#FF9900', '#109618', '#990099'],
      backgroundColor: options.backgroundColor || 'white',
      axisColor: options.axisColor || '#333333',
      labelColor: options.labelColor || '#666666',
      xLabel: options.xLabel || '',
      yLabel: options.yLabel || '',
      title: options.title || '',
      showGrid: options.showGrid !== undefined ? options.showGrid : false,
      gridColor: options.gridColor || '#EEEEEE',
      lineWidth: options.lineWidth || 2,
      dotSize: options.dotSize || 0,
      maxPoints: options.maxPoints || 100,
      streaming: options.streaming !== undefined ? options.streaming : false,
      retainData: options.retainData !== undefined ? options.retainData : false,
      viewWindowSize: options.viewWindowSize || 50,
      autoScrollWithNewData: options.autoScrollWithNewData !== undefined ? options.autoScrollWithNewData : true
    };
    
    // Set canvas dimensions
    this.canvas.width = this.options.width;
    this.canvas.height = this.options.height;
    
    // Calculate plotting area
    this.plotArea = {
      x: this.options.padding * 1.5,
      y: this.options.padding,
      width: this.options.width - this.options.padding * 2.5,
      height: this.options.height - this.options.padding * 2
    };
    
    // Initialize data
    this.datasets = [];
    
    // Initialize view window (for scrolling through data)
    this.viewWindow = {
      start: 0,
      size: this.options.viewWindowSize
    };
    
    // Track if we should auto-scroll with new data
    this.autoScroll = this.options.autoScrollWithNewData;
  }
  
  addDataset(data, label = '') {
    const colorIndex = this.datasets.length % this.options.lineColors.length;
    this.datasets.push({
      data: data,
      label: label,
      color: this.options.lineColors[colorIndex]
    });
    return this;
  }
  
  addPoint(datasetIndex, point) {
    if (datasetIndex < 0 || datasetIndex >= this.datasets.length) {
      console.error('Dataset index out of bounds');
      return this;
    }
    
    const dataset = this.datasets[datasetIndex];
    dataset.data.push(point);
    
    // If streaming is enabled and we're not retaining data, remove oldest points
    if (this.options.streaming && !this.options.retainData && dataset.data.length > this.options.maxPoints) {
      dataset.data = dataset.data.slice(-this.options.maxPoints);
    }
    
    // If retaining data and streaming, adjust view window to show most recent data
    // but only if autoScroll is enabled
    if (this.options.streaming && this.options.retainData && this.autoScroll) {
      const maxIdx = dataset.data.length - 1;
      if (maxIdx >= this.options.viewWindowSize) {
        this.viewWindow.start = maxIdx - this.options.viewWindowSize + 1;
      }
    }
    
    return this;
  }
  
  clear() {
    this.datasets = [];
    this.ctx.clearRect(0, 0, this.options.width, this.options.height);
    this.viewWindow.start = 0;
    this.autoScroll = this.options.autoScrollWithNewData;
    return this;
  }
  
  // Clear a specific dataset
  clearDataset(datasetIndex) {
    if (datasetIndex >= 0 && datasetIndex < this.datasets.length) {
      this.datasets[datasetIndex].data = [];
    }
    this.viewWindow.start = 0;
    this.autoScroll = this.options.autoScrollWithNewData;
    return this;
  }
  
  // Enable auto-scrolling with new data
  enableAutoScroll() {
    this.autoScroll = true;
    return this;
  }
  
  // Disable auto-scrolling with new data
  disableAutoScroll() {
    this.autoScroll = false;
    return this;
  }
  
  // Toggle auto-scrolling with new data
  toggleAutoScroll() {
    this.autoScroll = !this.autoScroll;
    return this;
  }
  
  // Scroll view window
  scroll(steps) {
    // Find maximum dataset length
    let maxDataLength = 0;
    this.datasets.forEach(dataset => {
      maxDataLength = Math.max(maxDataLength, dataset.data.length);
    });
    
    // Calculate new start position
    const newStart = this.viewWindow.start + steps;
    
    // Ensure we don't scroll past the data
    if (newStart >= 0 && newStart <= Math.max(0, maxDataLength - this.viewWindow.size)) {
      this.viewWindow.start = newStart;
      // Disable auto-scroll when manually scrolling backward
      if (steps < 0) {
        this.autoScroll = false;
      }
    }
    
    return this;
  }
  
  // Jump to specific position
  scrollTo(position) {
    // Find maximum dataset length
    let maxDataLength = 0;
    this.datasets.forEach(dataset => {
      maxDataLength = Math.max(maxDataLength, dataset.data.length);
    });
    
    // Check if position is at the latest data
    const isAtEnd = position >= maxDataLength - this.viewWindow.size;
    
    // Validate position
    if (position >= 0 && position <= Math.max(0, maxDataLength - this.viewWindow.size)) {
      this.viewWindow.start = position;
      
      // If not scrolling to the end, disable auto-scroll
      if (!isAtEnd) {
        this.autoScroll = false;
      }
    }
    
    return this;
  }
  
  // Scroll to the beginning of the data
  scrollToStart() {
    this.viewWindow.start = 0;
    this.autoScroll = false;
    return this;
  }
  
  // Scroll to the end of the data (most recent)
  scrollToEnd() {
    let maxDataLength = 0;
    this.datasets.forEach(dataset => {
      maxDataLength = Math.max(maxDataLength, dataset.data.length);
    });
    
    this.viewWindow.start = Math.max(0, maxDataLength - this.viewWindow.size);
    this.autoScroll = true;
    return this;
  }
  
  findMinMax() {
    if (this.datasets.length === 0) return { minX: 0, maxX: 10, minY: 0, maxY: 10 };
    
    let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
    
    this.datasets.forEach(dataset => {
      // Only consider points within the current view window
      const start = this.options.retainData ? this.viewWindow.start : 0;
      const end = this.options.retainData ? 
        Math.min(start + this.viewWindow.size, dataset.data.length) : 
        dataset.data.length;
      
      for (let i = start; i < end; i++) {
        const point = dataset.data[i];
        if (point) {
          minX = Math.min(minX, point.x);
          maxX = Math.max(maxX, point.x);
          minY = Math.min(minY, point.y);
          maxY = Math.max(maxY, point.y);
        }
      }
    });
    
    // If no points in view, return default
    if (minX === Infinity) {
      return { minX: 0, maxX: 10, minY: 0, maxY: 10 };
    }
    
    // Add a small padding to the max/min values
    const rangeX = maxX - minX;
    const rangeY = maxY - minY;
    
    return {
      minX: minX - rangeX * 0.05,
      maxX: maxX + rangeX * 0.05,
      minY: minY - rangeY * 0.05,
      maxY: maxY + rangeY * 0.05
    };
  }
  
  scalePoint(point, minMax) {
    return {
      x: this.plotArea.x + (point.x - minMax.minX) / (minMax.maxX - minMax.minX) * this.plotArea.width,
      y: this.plotArea.y + this.plotArea.height - (point.y - minMax.minY) / (minMax.maxY - minMax.minY) * this.plotArea.height
    };
  }
  
  drawAxes(minMax) {
    const ctx = this.ctx;
    
    // Draw axes
    ctx.beginPath();
    ctx.strokeStyle = this.options.axisColor;
    ctx.lineWidth = 1;
    
    // X-axis
    ctx.moveTo(this.plotArea.x, this.plotArea.y + this.plotArea.height);
    ctx.lineTo(this.plotArea.x + this.plotArea.width, this.plotArea.y + this.plotArea.height);
    
    // Y-axis
    ctx.moveTo(this.plotArea.x, this.plotArea.y);
    ctx.lineTo(this.plotArea.x, this.plotArea.y + this.plotArea.height);
    
    ctx.stroke();
    
    // Draw grid if enabled
    if (this.options.showGrid) {
      ctx.beginPath();
      ctx.strokeStyle = this.options.gridColor;
      ctx.lineWidth = 1;
      
      // Number of grid lines
      const gridLinesX = 5;
      const gridLinesY = 5;
      
      // X-axis grid lines
      for (let i = 1; i < gridLinesX; i++) {
        const x = this.plotArea.x + (i / gridLinesX) * this.plotArea.width;
        ctx.moveTo(x, this.plotArea.y);
        ctx.lineTo(x, this.plotArea.y + this.plotArea.height);
      }
      
      // Y-axis grid lines
      for (let i = 1; i < gridLinesY; i++) {
        const y = this.plotArea.y + (i / gridLinesY) * this.plotArea.height;
        ctx.moveTo(this.plotArea.x, y);
        ctx.lineTo(this.plotArea.x + this.plotArea.width, y);
      }
      
      ctx.stroke();
    }
    
    // Draw labels and ticks
    ctx.fillStyle = this.options.labelColor;
    ctx.font = '10px Arial';
    ctx.textAlign = 'center';
    
    // X-axis ticks and labels
    const ticksX = 5;
    for (let i = 0; i <= ticksX; i++) {
      const x = this.plotArea.x + (i / ticksX) * this.plotArea.width;
      const value = minMax.minX + (i / ticksX) * (minMax.maxX - minMax.minX);
      
      // Draw tick
      ctx.beginPath();
      ctx.moveTo(x, this.plotArea.y + this.plotArea.height);
      ctx.lineTo(x, this.plotArea.y + this.plotArea.height + 5);
      ctx.stroke();
      
      // Draw label
      ctx.fillText(value.toFixed(1), x, this.plotArea.y + this.plotArea.height + 15);
    }
    
    // Y-axis ticks and labels
    const ticksY = 5;
    ctx.textAlign = 'right';
    for (let i = 0; i <= ticksY; i++) {
      const y = this.plotArea.y + (1 - i / ticksY) * this.plotArea.height;
      const value = minMax.minY + (i / ticksY) * (minMax.maxY - minMax.minY);
      
      // Draw tick
      ctx.beginPath();
      ctx.moveTo(this.plotArea.x, y);
      ctx.lineTo(this.plotArea.x - 5, y);
      ctx.stroke();
      
      // Draw label
      ctx.fillText(value.toFixed(1), this.plotArea.x - 8, y + 3);
    }
    
    // Draw axis labels
    if (this.options.xLabel) {
      ctx.textAlign = 'center';
      ctx.fillText(this.options.xLabel, this.plotArea.x + this.plotArea.width / 2, this.options.height - 5);
    }
    
    if (this.options.yLabel) {
      ctx.save();
      ctx.translate(10, this.plotArea.y + this.plotArea.height / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.textAlign = 'center';
      ctx.fillText(this.options.yLabel, 0, 0);
      ctx.restore();
    }
    
    // Draw title
    if (this.options.title) {
      ctx.textAlign = 'center';
      ctx.font = 'bold 12px Arial';
      ctx.fillText(this.options.title, this.plotArea.x + this.plotArea.width / 2, 15);
    }
  }
  
  draw() {
    // Clear canvas
    this.ctx.fillStyle = this.options.backgroundColor;
    this.ctx.fillRect(0, 0, this.options.width, this.options.height);
    
    // If no data, just draw empty axes
    if (this.datasets.length === 0) {
      this.drawAxes({ minX: 0, maxX: 10, minY: 0, maxY: 10 });
      return this;
    }
    
    // Find min and max values
    const minMax = this.findMinMax();
    
    // Draw axes and grid
    this.drawAxes(minMax);
    
    // Draw each dataset
    this.datasets.forEach((dataset, datasetIndex) => {
      // Only consider points within the current view window
      const start = this.options.retainData ? this.viewWindow.start : 0;
      const end = this.options.retainData ? 
        Math.min(start + this.viewWindow.size, dataset.data.length) : 
        dataset.data.length;
      
      if (end - start < 1) return; // Skip if no data points in view
      
      this.ctx.strokeStyle = dataset.color;
      this.ctx.lineWidth = this.options.lineWidth;
      this.ctx.beginPath();
      
      // Draw data line
      let firstPoint = true;
      for (let i = start; i < end; i++) {
        const point = dataset.data[i];
        if (!point) continue;
        
        const scaledPoint = this.scalePoint(point, minMax);
        
        if (firstPoint) {
          this.ctx.moveTo(scaledPoint.x, scaledPoint.y);
          firstPoint = false;
        } else {
          this.ctx.lineTo(scaledPoint.x, scaledPoint.y);
        }
      }
      
      this.ctx.stroke();
      
      // Draw points if dotSize is set
      if (this.options.dotSize > 0) {
        this.ctx.fillStyle = dataset.color;
        
        for (let i = start; i < end; i++) {
          const point = dataset.data[i];
          if (!point) continue;
          
          const scaledPoint = this.scalePoint(point, minMax);
          
          this.ctx.beginPath();
          this.ctx.arc(scaledPoint.x, scaledPoint.y, this.options.dotSize, 0, Math.PI * 2);
          this.ctx.fill();
        }
      }
    });
    
    return this;
  }
  
  getDataLength() {
    let maxLength = 0;
    this.datasets.forEach(dataset => {
      maxLength = Math.max(maxLength, dataset.data.length);
    });
    return maxLength;
  }
}
)";
  
  webServer.send(200, "application/javascript", tinyLinePlotJS);
}

// Handle 404 - Just serve the main page instead
void handleNotFound() {
  handleRoot();
  debug_print("WEB", "Unknown URI requested - serving main page");
}

// Parse telemetry data string into JSON format
String parseTelemetryToJSON(const String& telemetryData) {
  // Initialize JSON structure
  String jsonResult = "{";
  
  // Extract values from the telemetry string
  float gForce = 0.0f;
  int motor1Throttle = 0;
  int motor2Throttle = 0;
  float accelUsed = 0.0f;
  int rcThrottle = 0;
  int rcSteering = 0;
  float accelX = 0.0f;
  float accelY = 0.0f;
  float accelZ = 0.0f;
  
  // Accelerometer data
  int rawAccelPos = telemetryData.indexOf("Raw Accel G:");
  if (rawAccelPos >= 0) {
    gForce = telemetryData.substring(rawAccelPos + 12).toFloat();
  }
  
  // Parse individual accelerometer X, Y, Z values
  int accelXYZPos = telemetryData.indexOf("Accel X:");
  if (accelXYZPos >= 0) {
    // Find Y position to determine the end of X value
    int yPos = telemetryData.indexOf("Y:", accelXYZPos);
    if (yPos > 0) {
      accelX = telemetryData.substring(accelXYZPos + 8, yPos - 1).toFloat();
      
      // Find Z position to determine the end of Y value
      int zPos = telemetryData.indexOf("Z:", yPos);
      if (zPos > 0) {
        accelY = telemetryData.substring(yPos + 3, zPos - 1).toFloat();
        
        // Find end of Z value (usually "Used value:" or end of line)
        int usedPos = telemetryData.indexOf("Used value:", zPos);
        if (usedPos > 0) {
          accelZ = telemetryData.substring(zPos + 3, usedPos - 1).toFloat();
        } else {
          // Try to find end of line if "Used value:" is not found
          int endOfLine = telemetryData.indexOf('\n', zPos);
          if (endOfLine > 0) {
            accelZ = telemetryData.substring(zPos + 3, endOfLine).toFloat();
          } else {
            // Just take the rest of the string
            accelZ = telemetryData.substring(zPos + 3).toFloat();
          }
        }
      }
    }
  }
  
  // RC Throttle data
  int throttlePos = telemetryData.indexOf("RC Throttle:");
  if (throttlePos >= 0) {
    rcThrottle = telemetryData.substring(throttlePos + 12).toInt();
    motor1Throttle = rcThrottle; // Assume same for both motors unless specified
    motor2Throttle = rcThrottle; // Assume same for both motors unless specified
  }
  
  // RC Steering data
  int steeringPos = telemetryData.indexOf("RC Steering:");
  if (steeringPos >= 0) {
    rcSteering = telemetryData.substring(steeringPos + 12).toInt();
  }
  
  // Motor PWM values for more accurate throttle percentage
  int motor1PWMPos = telemetryData.indexOf("Motor1 PWM:");
  if (motor1PWMPos >= 0) {
    int pwmValue = telemetryData.substring(motor1PWMPos + 11).toInt();
    // Convert PWM (1500-2000) to percentage (0-100)
    if (pwmValue > 1500) {
      motor1Throttle = (pwmValue - 1500) / 5; // 500 range maps to 0-100%
    }
  }
  
  int motor2PWMPos = telemetryData.indexOf("Motor2 PWM:");
  if (motor2PWMPos >= 0) {
    int pwmValue = telemetryData.substring(motor2PWMPos + 11).toInt();
    // Convert PWM (1500-2000) to percentage (0-100)
    if (pwmValue > 1500) {
      motor2Throttle = (pwmValue - 1500) / 5; // 500 range maps to 0-100%
    }
  }
  
  // Get the accelerometer used value from X: Y: Z: Used value: format
  int usedValuePos = telemetryData.indexOf("Used value:");
  if (usedValuePos >= 0) {
    accelUsed = telemetryData.substring(usedValuePos + 11).toFloat();
  }
  
  // Build JSON object with all values
  jsonResult += "\"gForce\":" + String(gForce, 2) + ",";
  jsonResult += "\"motor1Throttle\":" + String(motor1Throttle) + ",";
  jsonResult += "\"motor2Throttle\":" + String(motor2Throttle) + ",";
  jsonResult += "\"accelUsed\":" + String(accelUsed, 2) + ",";
  jsonResult += "\"rcThrottle\":" + String(rcThrottle) + ",";
  jsonResult += "\"rcSteering\":" + String(rcSteering) + ",";
  jsonResult += "\"accelX\":" + String(accelX, 3) + ",";
  jsonResult += "\"accelY\":" + String(accelY, 3) + ",";
  jsonResult += "\"accelZ\":" + String(accelZ, 3);
  
  // Add additional data that might be useful
  
  // Battery voltage if available
  int batteryPos = telemetryData.indexOf("Battery:");
  if (batteryPos >= 0) {
    float battery = telemetryData.substring(batteryPos + 9).toFloat();
    jsonResult += ",\"battery\":" + String(battery, 2);
  }
  
  // RPM if calculated (calculated in spin_control.cpp)
  int rpm = 0;
  int rpmPos = telemetryData.indexOf("RPM:");
  if (rpmPos >= 0) {
    rpm = telemetryData.substring(rpmPos + 4).toInt();
  } else {
    // Calculate RPM from G-force using the formula: RPM = sqrt(G / (0.00001118 * r))
    float radius = 10.0; // Default radius in cm if not specified
    
    // Get actual radius if available
    int radiusPos = telemetryData.indexOf("Radius:");
    if (radiusPos >= 0) {
      radius = telemetryData.substring(radiusPos + 8).toFloat();
    }
    
    // Use the accelUsed value which already has the zero G offset subtracted
    if (accelUsed > 0.1) { // Only calculate if G-force is significant
      rpm = sqrt(accelUsed / (0.00001118 * radius));
    }
  }
  jsonResult += ",\"rpm\":" + String(rpm);
  
  // Radius setting
  int radiusPos = telemetryData.indexOf("Radius:");
  if (radiusPos >= 0) {
    float radius = telemetryData.substring(radiusPos + 8).toFloat();
    jsonResult += ",\"radius\":" + String(radius, 2);
  }
  
  jsonResult += "}";
  return jsonResult;
}

// Function to be called from debug_handler.cpp to update the web data
void update_web_data(const String& telemetry, const String& logs) {
  portENTER_CRITICAL(&webDataMux);
  strncpy(webTelemetryBuffer, telemetry.c_str(), sizeof(webTelemetryBuffer) - 1);
  webTelemetryBuffer[sizeof(webTelemetryBuffer) - 1] = '\0';
  
  strncpy(webLogBuffer, logs.c_str(), sizeof(webLogBuffer) - 1);
  webLogBuffer[sizeof(webLogBuffer) - 1] = '\0';
  portEXIT_CRITICAL(&webDataMux);
}

// Web server task function to run in separate thread
void web_server_task(void *parameter) {
  // Reduce WiFi power to minimum to avoid interference
  WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
  
  // Configure access point
  String wifiMsg = "Setting up WiFi access point: " + String(ssid);
  debug_print_safe("WEB", wifiMsg);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  
  String ipMsg = "Web server started with IP: " + myIP.toString();
  debug_print_safe("WEB", ipMsg);
  
  // Set up web server handlers
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/telemetry", HTTP_GET, handleTelemetry);
  webServer.on("/logs", HTTP_GET, handleLogs);
  webServer.on("/clear", HTTP_POST, handleClear);
  webServer.on("/eeprom", HTTP_GET, handleEEPROM);
  webServer.on("/TinyLinePlot.js", HTTP_GET, handleTinyLinePlotJS);
  
  // Serve main page for any requested path
  webServer.onNotFound(handleNotFound);
  
  // Start the server
  webServer.begin();
  server_running = true;
  
  String readyMsg = "Web interface is ready at http://" + myIP.toString();
  debug_print_safe("WEB", readyMsg);
  
  // Main loop for the web server task
  while (true) {
    // Process web server requests
    webServer.handleClient();
    
    // Small delay to avoid hogging CPU
    delay(10);
  }
}

// Function to start the web server in a separate task
void start_web_server() {
  // Check if server is already running
  if (server_running) {
    debug_print("WEB", "Web server is already running");
    return;
  }
  
  // Create task with increased stack size
  xTaskCreatePinnedToCore(
    web_server_task,   // Task function
    "WebServerTask",   // Task name
    WEB_SERVER_TASK_STACK_SIZE,  // Stack size 
    NULL,              // Parameters
    1,                 // Priority
    NULL,              // Task handle
    1                  // Core (1 = second core)
  );
}

// Function to initialize the web server (API compatibility wrapper)
void init_web_server() {
  start_web_server();
} 
