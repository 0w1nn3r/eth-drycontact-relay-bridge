#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include <ESP32WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <Preferences.h>

#include "config.h"
#include "Display.h"

// Global variables
ESP32WebServer server(WEB_PORT);
OperationMode currentMode = MODE_UNDEFINED;
bool ethernetConnected = false;
unsigned long lastHeartbeat = 0;
unsigned long lastLedBlink = 0;
bool ledState = false;

// Display object
Display display;

// Mode-specific variables
bool dryContactState = false;
unsigned long dryContactChangeTime = 0;
bool relayState = false;
unsigned long relayPulseEndTime = 0;

// Network configuration
String deviceID = "";
String receiverIP = "";
bool useTCP = false;  // false for UDP, true for TCP

// Mode jumper state
bool jumperModeDetected = false;

// Function prototypes
void setupHardware();
void setupNetwork();
void setupWebServer();
void handleRoot();
void handleAPI();
void handleConfig();
void handleMode();
void handleStatus();
void handleDiscovery();
void saveConfiguration();
void loadConfiguration();
void generateDeviceID();
void updateStatusLED();
void handleDryContact();
void handleRelay();
void sendHeartbeat();
void processIncomingMessage(const String& message, const String& senderIP);
void sendRelayCommand(const String& targetIP, bool activate);
void setupDryContactSender();
void setupRelayReceiver();
void detectModeFromJumper();

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("\n=== " BOARD_NAME " ===");
    Serial.println("Firmware Version: " FIRMWARE_VERSION);
    
    setupHardware();
    detectModeFromJumper();
    loadConfiguration();
    generateDeviceID();
    
    // Initialize display
    if (display.init()) {
        display.setDeviceID(deviceID);
        display.setMode(currentMode);
        display.showConnecting();
    }
    
    Serial.println("Device ID: " + deviceID);
    Serial.println("Operation Mode: " + String(currentMode));
    Serial.println("Mode Jumper: " + String(jumperModeDetected ? "Detected" : "Not detected"));
    
    setupNetwork();
    setupWebServer();
    
    // Initialize mode-specific functionality
    switch (currentMode) {
        case MODE_DRY_CONTACT_SENDER:
            setupDryContactSender();
            break;
        case MODE_RELAY_RECEIVER:
            setupRelayReceiver();
            break;
        default:
            Serial.println("WARNING: No mode configured, please configure via web interface");
            break;
    }
    
    Serial.println("Setup completed");
}

void loop() {
    // Handle web server clients
    server.handleClient();
    
    // Update status LED
    updateStatusLED();
    
    // Update display
    if (display.isAvailable()) {
        display.update();
    }
    
    // Handle mode-specific operations
    switch (currentMode) {
        case MODE_DRY_CONTACT_SENDER:
            handleDryContact();
            break;
        case MODE_RELAY_RECEIVER:
            handleRelay();
            break;
        default:
            break;
    }
    
    // Send periodic heartbeat
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
        sendHeartbeat();
        lastHeartbeat = millis();
    }
    
    delay(10);
}

void setupHardware() {
    Serial.println("Initializing hardware...");
    
    // Initialize status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    
    // Initialize mode jumper pin
    pinMode(MODE_JUMPER_PIN, INPUT_PULLUP);
    
    // Initialize mode-specific pins
    pinMode(DRY_CONTACT_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    
    // Set relay initial state
    if (RELAY_ACTIVE_LOW) {
        digitalWrite(RELAY_PIN, HIGH);  // Relay off (active low)
    } else {
        digitalWrite(RELAY_PIN, LOW);   // Relay off (active high)
    }
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    Serial.println("Hardware initialized");
}

void setupNetwork() {
    Serial.println("Initializing Ethernet...");
    
    // Configure Ethernet
    ETH.begin();
    
    // Wait for Ethernet connection
    Serial.print("Connecting to Ethernet...");
    int retryCount = 0;
    while (!ethernetConnected && retryCount < 30) {
        ethernetConnected = (ETH.linkStatus() == ETH_LINK_UP);
        if (ethernetConnected) {
            Serial.println(" connected!");
            Serial.println("IP Address: " + ETH.localIP().toString());
            Serial.println("MAC Address: " + ETH.macAddress());
            
            // Update display with connection info
            if (display.isAvailable()) {
                display.setEthernetConnected(true);
                display.setLocalIP(ETH.localIP().toString());
                display.showConnected();
            }
            
            // Start mDNS
            if (MDNS.begin(MDNS_NAME)) {
                Serial.println("mDNS responder started");
                MDNS.addService("http", "tcp", WEB_PORT);
                MDNS.addService("eth-relay", "udp", UDP_PORT);
            } else {
                Serial.println("Error setting up MDNS responder!");
            }
        } else {
            Serial.print(".");
            delay(1000);
            retryCount++;
        }
    }
    
    if (!ethernetConnected) {
        Serial.println(" failed to connect to Ethernet!");
        Serial.println("Continuing without network connectivity...");
    }
}

void setupWebServer() {
    Serial.println("Setting up web server...");
    
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api", HTTP_GET, handleAPI);
    server.on("/config", HTTP_GET, handleConfig);
    server.on("/config", HTTP_POST, handleConfig);
    server.on("/mode", HTTP_GET, handleMode);
    server.on("/mode", HTTP_POST, handleMode);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/discovery", HTTP_GET, handleDiscovery);
    
    server.begin();
    Serial.println("HTTP server started");
}

void generateDeviceID() {
    uint64_t chipid = ESP.getEfuseMac();
    deviceID = "ETH-" + String((uint16_t)(chipid >> 32), HEX) + 
               String((uint16_t)(chipid >> 16), HEX) + 
               String((uint16_t)(chipid), HEX);
    deviceID.toUpperCase();
}

void updateStatusLED() {
    unsigned long currentMillis = millis();
    unsigned long blinkInterval;
    
    if (!ethernetConnected) {
        blinkInterval = LED_BLINK_ERROR_MS;
    } else if (currentMode == MODE_UNDEFINED) {
        blinkInterval = LED_BLINK_CONNECTING_MS;
    } else {
        blinkInterval = LED_BLINK_NORMAL_MS;
    }
    
    if (currentMillis - lastLedBlink > blinkInterval) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        lastLedBlink = currentMillis;
    }
}

void loadConfiguration() {
    // Load operation mode from EEPROM
    currentMode = (OperationMode)EEPROM.read(EEPROM_MODE_ADDR);
    
    // Load other configuration settings
    // This would be expanded to load receiver IP, protocol choice, etc.
    Preferences preferences;
    preferences.begin("eth-relay", false);
    receiverIP = preferences.getString("receiverIP", "");
    useTCP = preferences.getBool("useTCP", false);
    preferences.end();
    
    Serial.println("Configuration loaded:");
    Serial.println("  Mode: " + String(currentMode));
    Serial.println("  Receiver IP: " + receiverIP);
    Serial.println("  Use TCP: " + String(useTCP ? "Yes" : "No"));
}

void saveConfiguration() {
    // Save operation mode to EEPROM
    EEPROM.write(EEPROM_MODE_ADDR, (uint8_t)currentMode);
    EEPROM.commit();
    
    // Save other configuration settings
    Preferences preferences;
    preferences.begin("eth-relay", false);
    preferences.putString("receiverIP", receiverIP);
    preferences.putBool("useTCP", useTCP);
    preferences.end();
    
    Serial.println("Configuration saved");
}

void setupDryContactSender() {
    Serial.println("Configuring as Dry Contact Sender");
    dryContactState = (digitalRead(DRY_CONTACT_PIN) == LOW);
    dryContactChangeTime = millis();
}

void setupRelayReceiver() {
    Serial.println("Configuring as Relay Receiver");
}

void detectModeFromJumper() {
    Serial.println("Detecting mode from jumper...");
    
    // Read jumper state (LOW = Sender, HIGH = Receiver)
    bool jumperState = digitalRead(MODE_JUMPER_PIN);
    
    if (jumperState == LOW) {
        // Jumper is grounded (LOW) - Sender mode
        currentMode = MODE_DRY_CONTACT_SENDER;
        jumperModeDetected = true;
        Serial.println("Jumper detected: LOW -> Sender mode");
    } else {
        // Jumper is open (HIGH) - Receiver mode
        currentMode = MODE_RELAY_RECEIVER;
        jumperModeDetected = true;
        Serial.println("Jumper detected: HIGH -> Receiver mode");
    }
    
    // Only override EEPROM if jumper is detected
    if (jumperModeDetected) {
        Serial.println("Mode set by jumper, overriding EEPROM setting");
    }
}

void handleDryContact() {
    bool currentState = (digitalRead(DRY_CONTACT_PIN) == LOW);
    
    if (currentState != dryContactState) {
        // Debounce
        if (millis() - dryContactChangeTime > DRY_CONTACT_DEBOUNCE_MS) {
            dryContactState = currentState;
            dryContactChangeTime = millis();
            
            Serial.println("Dry contact state changed: " + String(dryContactState ? "CLOSED" : "OPEN"));
            
            // Update display
            if (display.isAvailable()) {
                display.setInputState(dryContactState);
            }
            
            // Send state change to receiver
            if (receiverIP.length() > 0) {
                sendRelayCommand(receiverIP, dryContactState);
            } else {
                Serial.println("WARNING: No receiver IP configured");
            }
        }
    }
}

void handleRelay() {
    // Handle relay pulse timing
    if (relayState && millis() > relayPulseEndTime) {
        relayState = false;
        if (RELAY_ACTIVE_LOW) {
            digitalWrite(RELAY_PIN, HIGH);  // Relay off
        } else {
            digitalWrite(RELAY_PIN, LOW);   // Relay off
        }
        Serial.println("Relay deactivated (pulse completed)");
    }
}

void sendRelayCommand(const String& targetIP, bool activate) {
    if (!ethernetConnected) {
        Serial.println("Cannot send relay command: Ethernet not connected");
        return;
    }
    
    // Create JSON message
    JsonDocument doc;
    doc["device_id"] = deviceID;
    doc["command"] = activate ? "relay_on" : "relay_off";
    doc["timestamp"] = millis();
    doc["protocol_version"] = PROTOCOL_VERSION;
    
    String message;
    serializeJson(doc, message);
    
    if (useTCP) {
        // Send via TCP
        HTTPClient http;
        http.begin("http://" + targetIP + ":" + String(TCP_PORT) + "/command");
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(message);
        
        if (httpResponseCode > 0) {
            Serial.println("TCP command sent successfully. Response code: " + String(httpResponseCode));
        } else {
            Serial.println("Error sending TCP command: " + String(httpResponseCode));
        }
        http.end();
    } else {
        // Send via UDP (would need WiFiUDP implementation)
        Serial.println("UDP command: " + message + " to " + targetIP + ":" + String(UDP_PORT));
        // TODO: Implement UDP sending
    }
}

void sendHeartbeat() {
    if (!ethernetConnected || currentMode == MODE_UNDEFINED) {
        return;
    }
    
    JsonDocument doc;
    doc["device_id"] = deviceID;
    doc["mode"] = currentMode;
    doc["status"] = "online";
    doc["timestamp"] = millis();
    doc["protocol_version"] = PROTOCOL_VERSION;
    
    String message;
    serializeJson(doc, message);
    
    // Send heartbeat via broadcast for device discovery
    Serial.println("Heartbeat: " + message);
    // TODO: Implement UDP broadcast
}

void processIncomingMessage(const String& message, const String& senderIP) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.println("Error parsing incoming message: " + String(error.c_str()));
        return;
    }
    
    String command = doc["command"] | "";
    String senderDeviceID = doc["device_id"] | "";
    
    Serial.println("Received command: " + command + " from " + senderDeviceID);
    
    if (currentMode == MODE_RELAY_RECEIVER) {
        if (command == "relay_on") {
            relayState = true;
            relayPulseEndTime = millis() + RELAY_PULSE_MS;
            if (RELAY_ACTIVE_LOW) {
                digitalWrite(RELAY_PIN, LOW);  // Relay on
            } else {
                digitalWrite(RELAY_PIN, HIGH); // Relay on
            }
            Serial.println("Relay activated");
            if (display.isAvailable()) {
                display.setOutputState(true);
            }
        } else if (command == "relay_off") {
            relayState = false;
            if (RELAY_ACTIVE_LOW) {
                digitalWrite(RELAY_PIN, HIGH);  // Relay off
            } else {
                digitalWrite(RELAY_PIN, LOW);   // Relay off
            }
            Serial.println("Relay deactivated");
            if (display.isAvailable()) {
                display.setOutputState(false);
            }
        }
    }
}

// Web server handlers
void handleRoot() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>)" + String(BOARD_NAME) + R"(</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .container { max-width: 600px; margin: 0 auto; }
        .status { background: #f0f0f0; padding: 10px; margin: 10px 0; border-radius: 5px; }
        .button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; }
        .button:hover { background: #0056b3; }
        .form-group { margin: 10px 0; }
        label { display: block; margin-bottom: 5px; }
        input, select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>)" + String(BOARD_NAME) + R"(</h1>
        <div class="status">
            <h3>Device Status</h3>
            <p><strong>Device ID:</strong> )" + deviceID + R"(</p>
            <p><strong>Firmware:</strong> )" + FIRMWARE_VERSION + R"(</p>
            <p><strong>Mode:</strong> <span id="mode">)" + String(currentMode) + R"(</span>)" + String(jumperModeDetected ? " (Jumper Set)" : "") + R"(</p>
            <p><strong>Ethernet:</strong> <span id="ethernet">)" + String(ethernetConnected ? "Connected" : "Disconnected") + R"(</span></p>
            <p><strong>IP Address:</strong> )" + ETH.localIP().toString() + R"(</p>
        </div>
        
        <div class="form-group">
            <h3>Configuration</h3>
            <form method="POST" action="/mode">
                <label for="operationMode">Operation Mode:</label>
                <select name="mode" id="operationMode">
                    <option value="0">Not Configured</option>
                    <option value="1">Dry Contact Sender</option>
                    <option value="2">Relay Receiver</option>
                </select>
                <br><br>
                <label for="receiverIP">Receiver IP (for Sender mode):</label>
                <input type="text" id="receiverIP" name="receiverIP" placeholder="192.168.1.100" value=")" + receiverIP + R"(">
                <br><br>
                <label>
                    <input type="checkbox" name="useTCP" )" + String(useTCP ? "checked" : "") + R"(>
                    Use TCP instead of UDP
                </label>
                <br><br>
                <button type="submit" class="button">Save Configuration</button>
            </form>
        </div>
    </div>
    
    <script>
        // Update status every 2 seconds
        setInterval(function() {
            fetch('/api')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('mode').textContent = data.mode_text || 'Unknown';
                    document.getElementById('ethernet').textContent = data.ethernet_connected ? 'Connected' : 'Disconnected';
                })
                .catch(error => console.log('Error:', error));
        }, 2000);
    </script>
</body>
</html>
)";
    
    server.send(200, "text/html", html);
}

void handleAPI() {
    JsonDocument doc;
    doc["device_id"] = deviceID;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["mode"] = currentMode;
    doc["mode_text"] = currentMode == MODE_DRY_CONTACT_SENDER ? "Dry Contact Sender" : 
                      currentMode == MODE_RELAY_RECEIVER ? "Relay Receiver" : "Not Configured";
    doc["jumper_mode"] = jumperModeDetected;
    doc["ethernet_connected"] = ethernetConnected;
    doc["ip_address"] = ETH.localIP().toString();
    doc["mac_address"] = ETH.macAddress();
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    
    if (currentMode == MODE_DRY_CONTACT_SENDER) {
        doc["dry_contact_state"] = dryContactState;
        doc["receiver_ip"] = receiverIP;
        doc["use_tcp"] = useTCP;
    } else if (currentMode == MODE_RELAY_RECEIVER) {
        doc["relay_state"] = relayState;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleConfig() {
    if (server.method() == HTTP_POST) {
        if (server.hasArg("receiverIP")) {
            receiverIP = server.arg("receiverIP");
        }
        if (server.hasArg("useTCP")) {
            useTCP = server.arg("useTCP") == "on";
        }
        saveConfiguration();
        server.send(200, "text/plain", "Configuration saved");
    } else {
        JsonDocument doc;
        doc["receiver_ip"] = receiverIP;
        doc["use_tcp"] = useTCP;
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
}

void handleMode() {
    if (server.method() == HTTP_POST) {
        if (server.hasArg("mode")) {
            int newMode = server.arg("mode").toInt();
            if (newMode >= MODE_UNDEFINED && newMode <= MODE_RELAY_RECEIVER) {
                currentMode = (OperationMode)newMode;
                saveConfiguration();
                
                // Reinitialize mode-specific functionality
                switch (currentMode) {
                    case MODE_DRY_CONTACT_SENDER:
                        setupDryContactSender();
                        break;
                    case MODE_RELAY_RECEIVER:
                        setupRelayReceiver();
                        break;
                }
                
                server.send(200, "text/plain", "Mode updated");
            } else {
                server.send(400, "text/plain", "Invalid mode");
            }
        } else {
            server.send(400, "text/plain", "Missing mode parameter");
        }
    } else {
        JsonDocument doc;
        doc["current_mode"] = currentMode;
        doc["modes"] = JsonArray();
        JsonArray modes = doc["modes"].to<JsonArray>();
        modes.add("Not Configured");
        modes.add("Dry Contact Sender");
        modes.add("Relay Receiver");
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
}

void handleStatus() {
    JsonDocument doc;
    doc["status"] = "online";
    doc["device_id"] = deviceID;
    doc["mode"] = currentMode;
    doc["ethernet_connected"] = ethernetConnected;
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleDiscovery() {
    JsonDocument doc;
    doc["device_id"] = deviceID;
    doc["name"] = BOARD_NAME;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["mode"] = currentMode;
    doc["ip_address"] = ETH.localIP().toString();
    doc["protocol_version"] = PROTOCOL_VERSION;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}
