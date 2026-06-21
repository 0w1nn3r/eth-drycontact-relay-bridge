#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <Preferences.h>

#include "config.h"
#include "Display.h"
#include "WebServer.h"
#include "StateLogger.h"
#include "TimeManager.h"

// Global variables
OperationMode currentMode = MODE_UNDEFINED;
bool ethernetConnected = false;
unsigned long lastHeartbeat = 0;
unsigned long lastLedBlink = 0;
bool ledState = false;

// Time management
TimeManager timeManager;

// Mode-specific variables (dual channel)
bool dryContactState[2] = {false, false};
unsigned long dryContactChangeTime[2] = {0, 0};
bool relayState[2] = {false, false};
unsigned long relayPulseEndTime[2] = {0, 0};

// Network configuration
String deviceID = "";
String receiverIP = "";

// Mode jumper state
bool jumperModeDetected = false;

// UDP for network discovery
WiFiUDP udp;

// Pairing state (device-to-device: one paired sender drives both channels)
bool isPaired = false;
String pairedDeviceID = "";
String pairedDeviceIP = "";
unsigned long lastDiscoveryBroadcast = 0;
unsigned long lastDiscoveryScan = 0;
bool discoveryEnabled = true;

// State logger
StateLogger stateLogger;

// Channel names
String channel1Name = DEFAULT_CHANNEL1_NAME;
String channel2Name = DEFAULT_CHANNEL2_NAME;

// Factory reset button
unsigned long factoryResetButtonPressTime = 0;
bool factoryResetButtonPressed = false;
bool factoryResetBootDetection = false;

// Display and Web server objects
Display display;
WebServer webServer(deviceID, currentMode, ethernetConnected, jumperModeDetected,
                   isPaired, pairedDeviceID, pairedDeviceIP, receiverIP,
                   discoveryEnabled, dryContactState, relayState, channel1Name, channel2Name);

// Function prototypes
void setupHardware();
void setupNetwork();
void saveConfiguration();
void loadConfiguration();
void generateDeviceID();
void updateStatusLED();
void handleDryContact();
void handleRelay();
void sendHeartbeat();
void processIncomingMessage(const String& message, const String& senderIP);
void sendRelayCommand(const String& targetIP, int channel, bool activate);
void setupDryContactSender();
void setupRelayReceiver();
void detectModeFromJumper();
void sendDiscoveryBroadcast();
void scanForSenders();
void handleDiscoveryPacket(const String& message);
void pairWithDevice(const String& deviceID, const String& deviceIP);
void unpairDevice();
void loadPairingSettings();
void savePairingSettings();
void loadChannelNames();
void saveChannelNames();
void checkFactoryReset();
void performFactoryReset();
void handleFactoryResetButton();

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("\n=== " BOARD_NAME " ===");
    Serial.println("Firmware Version: " FIRMWARE_VERSION);
    
    setupHardware();
    detectModeFromJumper();
    checkFactoryReset();  // Check for factory reset button during boot
    loadConfiguration();
    generateDeviceID();
    loadPairingSettings();
    loadChannelNames();
    
    // Initialize state logger
    stateLogger.begin();
    
    // Initialize display
    if (display.init()) {
        display.setDeviceID(deviceID);
        display.setMode(currentMode);
        display.setChannelNames(channel1Name, channel2Name);
        display.showConnecting();
    }
    
    Serial.println("Device ID: " + deviceID);
    Serial.println("Operation Mode: " + String(currentMode));
    Serial.println("Mode Jumper: " + String(jumperModeDetected ? "Detected" : "Not detected"));
    
    setupNetwork();
    
    // Initialize time manager
    timeManager.begin();
    
    // Setup web server
    webServer.setDisplay(&display);
    webServer.setStateLogger(&stateLogger);
    stateLogger.setTimeManager(&timeManager);
    webServer.setChannelNameSaveCallback([]() { saveChannelNames(); });
    webServer.begin();
    
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
    webServer.handleClient();
    
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
    
    // Handle discovery and pairing
    if (ethernetConnected && discoveryEnabled) {
        if (currentMode == MODE_DRY_CONTACT_SENDER) {
            // Senders broadcast their presence every 10 seconds
            if (millis() - lastDiscoveryBroadcast > 10000) {
                sendDiscoveryBroadcast();
                lastDiscoveryBroadcast = millis();
            }
        } else if (currentMode == MODE_RELAY_RECEIVER && !isPaired) {
            // Receivers scan for senders every 5 seconds until paired
            if (millis() - lastDiscoveryScan > 5000) {
                scanForSenders();
                lastDiscoveryScan = millis();
            }
        }
    }
    
    // Handle factory reset button
    handleFactoryResetButton();
    
    // Update time manager
    timeManager.update();
    
    // Update state logger
    stateLogger.update();
    
    delay(10);
}

void setupHardware() {
    Serial.println("Initializing hardware...");
    
    // Initialize status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    
    // Initialize mode jumper pin
    pinMode(MODE_JUMPER_PIN, INPUT_PULLUP);
    
    // Initialize factory reset button (BOOT button)
    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
    
    // Initialize mode-specific pins (dual channel)
    pinMode(DRY_CONTACT_PIN_1, INPUT_PULLUP);
    pinMode(DRY_CONTACT_PIN_2, INPUT_PULLUP);
    pinMode(RELAY_PIN_1, OUTPUT);
    pinMode(RELAY_PIN_2, OUTPUT);
    
    // Set relay initial states
    if (RELAY_ACTIVE_LOW) {
        digitalWrite(RELAY_PIN_1, HIGH);  // Relay off (active low)
        digitalWrite(RELAY_PIN_2, HIGH);  // Relay off (active low)
    } else {
        digitalWrite(RELAY_PIN_1, LOW);   // Relay off (active high)
        digitalWrite(RELAY_PIN_2, LOW);   // Relay off (active high)
    }
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    Serial.println("Hardware initialized");
}

void setupNetwork() {
    Serial.println("Initializing Ethernet with DHCP...");
    
    // Configure Ethernet with explicit DHCP settings
    ETH.begin();
    
    // Wait for Ethernet connection and DHCP assignment
    Serial.print("Connecting to Ethernet (DHCP)...");
    int retryCount = 0;
    while (!ethernetConnected && retryCount < 30) {
        ethernetConnected = ETH.linkUp();
        if (ethernetConnected) {
            Serial.println(" connected!");
            
            // Wait for DHCP to complete
            int dhcpRetry = 0;
            while (ETH.localIP().toString() == "0.0.0.0" && dhcpRetry < 10) {
                Serial.print("Waiting for DHCP...");
                delay(1000);
                dhcpRetry++;
            }
            
            if (ETH.localIP().toString() != "0.0.0.0") {
                Serial.println(" DHCP successful!");
                Serial.println("IP Address: " + ETH.localIP().toString());
                Serial.println("Subnet Mask: " + ETH.subnetMask().toString());
                Serial.println("Gateway: " + ETH.gatewayIP().toString());
                Serial.println("DNS Server: " + ETH.dnsIP().toString());
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
                Serial.println(" DHCP failed!");
                ethernetConnected = false;
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
    Preferences preferences;
    preferences.begin("eth-relay", false);
    receiverIP = preferences.getString("receiverIP", "");
    preferences.end();
    
    Serial.println("Configuration loaded:");
    Serial.println("  Mode: " + String(currentMode));
    Serial.println("  Receiver IP: " + receiverIP);
}

void saveConfiguration() {
    // Save operation mode to EEPROM
    EEPROM.write(EEPROM_MODE_ADDR, (uint8_t)currentMode);
    EEPROM.commit();
    
    // Save other configuration settings
    Preferences preferences;
    preferences.begin("eth-relay", false);
    preferences.putString("receiverIP", receiverIP);
    preferences.end();
    
    Serial.println("Configuration saved");
}

void setupDryContactSender() {
    Serial.println("Configuring as Dry Contact Sender");
    dryContactState[0] = (digitalRead(DRY_CONTACT_PIN_1) == LOW);
    dryContactState[1] = (digitalRead(DRY_CONTACT_PIN_2) == LOW);
    dryContactChangeTime[0] = millis();
    dryContactChangeTime[1] = millis();
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
    // Handle both channels
    for (int channel = 0; channel < 2; channel++) {
        int pin = (channel == 0) ? DRY_CONTACT_PIN_1 : DRY_CONTACT_PIN_2;
        bool currentState = (digitalRead(pin) == LOW);
        
        if (currentState != dryContactState[channel]) {
            // Debounce
            if (millis() - dryContactChangeTime[channel] > DRY_CONTACT_DEBOUNCE_MS) {
                bool oldState = dryContactState[channel];
                dryContactState[channel] = currentState;
                dryContactChangeTime[channel] = millis();
                
                Serial.println("Dry contact CH" + String(channel + 1) + " state changed: " + 
                              String(currentState ? "CLOSED" : "OPEN"));
                
                // Log state change
                stateLogger.logStateChange(channel + 1, "DRY_CONTACT", 
                                          String(oldState ? "CLOSED" : "OPEN"), 
                                          String(currentState ? "CLOSED" : "OPEN"));
                
                // Update display
                if (display.isAvailable()) {
                    display.setInputState(channel + 1, currentState);
                }
                
                // Send state change to receiver
                if (receiverIP.length() > 0) {
                    sendRelayCommand(receiverIP, channel + 1, currentState);
                } else {
                    Serial.println("WARNING: No receiver IP configured for CH" + String(channel + 1));
                }
            }
        }
    }
}

void handleRelay() {
    // Handle relay pulse timing for both channels
    for (int channel = 0; channel < 2; channel++) {
        if (relayState[channel] && relayPulseEndTime[channel] != 0 && millis() > relayPulseEndTime[channel]) {
            relayState[channel] = false;
            int pin = (channel == 0) ? RELAY_PIN_1 : RELAY_PIN_2;
            
            if (RELAY_ACTIVE_LOW) {
                digitalWrite(pin, HIGH);  // Relay off
            } else {
                digitalWrite(pin, LOW);   // Relay off
            }
            
            Serial.println("Relay CH" + String(channel + 1) + " deactivated (pulse completed)");
            
            // Log state change
            stateLogger.logStateChange(channel + 1, "RELAY", "ON", "OFF", "Pulse completed");
            
            if (display.isAvailable()) {
                display.setOutputState(channel + 1, false);
            }
        }
    }
}

void sendRelayCommand(const String& targetIP, int channel, bool activate) {
    if (!ethernetConnected) {
        Serial.println("Cannot send relay command: Ethernet not connected");
        return;
    }
    
    // Create JSON message
    JsonDocument doc;
    doc["device_id"] = deviceID;
    doc["command"] = activate ? "relay_on" : "relay_off";
    doc["channel"] = channel;
    doc["timestamp"] = millis();
    doc["protocol_version"] = PROTOCOL_VERSION;
    
    String message;
    serializeJson(doc, message);
    
    // Send via TCP
    HTTPClient http;
    http.begin("http://" + targetIP + ":" + String(WEB_PORT) + "/command");
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(message);
    
    if (httpResponseCode > 0) {
        Serial.println("TCP command sent successfully. Response code: " + String(httpResponseCode));
    } else {
        Serial.println("Error sending TCP command: " + String(httpResponseCode));
    }
    http.end();
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
    int channel = doc["channel"] | 1;  // Default to channel 1 if not specified
    
    Serial.println("Received command: " + command + " for CH" + String(channel) + " from " + senderDeviceID);
    
    if (currentMode == MODE_RELAY_RECEIVER && channel >= 1 && channel <= 2) {
        int channelIndex = channel - 1;
        int relayPin = (channel == 1) ? RELAY_PIN_1 : RELAY_PIN_2;

        // Security: only act on commands from the paired sender. Pairing is
        // device-to-device, so the single paired sender drives both channels.
        // The /command endpoint is unauthenticated, so without this check any
        // device on the LAN could POST and trip or silence the relay. Verify the
        // source IP matches the paired sender (and the device ID, when supplied)
        // before touching the relay.
        if (!isPaired || pairedDeviceIP.length() == 0) {
            Serial.println("Rejected command for CH" + String(channel) +
                           ": device is not paired");
            return;
        }
        if (senderIP != pairedDeviceIP) {
            Serial.println("Rejected command for CH" + String(channel) + " from " +
                           senderIP + ": does not match paired sender IP " +
                           pairedDeviceIP);
            return;
        }
        if (senderDeviceID.length() > 0 &&
            pairedDeviceID.length() > 0 &&
            senderDeviceID != pairedDeviceID) {
            Serial.println("Rejected command for CH" + String(channel) + " from device " +
                           senderDeviceID + ": does not match paired device " +
                           pairedDeviceID);
            return;
        }

        if (command == "relay_on") {
            relayState[channelIndex] = true;
            relayPulseEndTime[channelIndex] = RELAY_LATCHING ? 0 : (millis() + RELAY_PULSE_MS);
            if (RELAY_ACTIVE_LOW) {
                digitalWrite(relayPin, LOW);  // Relay on
            } else {
                digitalWrite(relayPin, HIGH); // Relay on
            }
            Serial.println("Relay CH" + String(channel) + " activated");
            
            // Log state change
            stateLogger.logStateChange(channel, "RELAY", "OFF", "ON", "Command received");
            
            if (display.isAvailable()) {
                display.setOutputState(channel, true);
            }
        } else if (command == "relay_off") {
            relayState[channelIndex] = false;
            if (RELAY_ACTIVE_LOW) {
                digitalWrite(relayPin, HIGH);  // Relay off
            } else {
                digitalWrite(relayPin, LOW);   // Relay off
            }
            Serial.println("Relay CH" + String(channel) + " deactivated");
            
            // Log state change
            stateLogger.logStateChange(channel, "RELAY", "ON", "OFF", "Command received");
            
            if (display.isAvailable()) {
                display.setOutputState(channel, false);
            }
        }
    }
}

// Discovery and pairing functions






// Discovery and pairing functions
void sendDiscoveryBroadcast() {
    JsonDocument doc;
    doc["type"] = "discovery";
    doc["device_id"] = deviceID;
    doc["mode"] = currentMode;
    doc["mode_text"] = currentMode == MODE_DRY_CONTACT_SENDER ? "Sender" : "Receiver";
    doc["ip_address"] = ETH.localIP().toString();
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["protocol_version"] = PROTOCOL_VERSION;
    doc["timestamp"] = millis();
    
    String message;
    serializeJson(doc, message);
    
    // Broadcast discovery message
    udp.beginPacket("255.255.255.255", DEVICE_DISCOVERY_PORT);
    udp.print(message);
    udp.endPacket();
    
    Serial.println("Discovery broadcast sent: " + message);
}

void scanForSenders() {
    Serial.println("Scanning for senders...");
    
    // Listen for discovery responses
    udp.begin(DEVICE_DISCOVERY_PORT);
    
    unsigned long scanStart = millis();
    while (millis() - scanStart < 3000) { // Scan for 3 seconds
        int packetSize = udp.parsePacket();
        if (packetSize > 0) {
            char packetBuffer[512];
            int len = udp.read(packetBuffer, packetSize - 1);
            packetBuffer[len] = '\0';
            
            String message = String(packetBuffer);
            Serial.println("Received discovery packet: " + message);
            
            handleDiscoveryPacket(message);
        }
        delay(10);
    }
    
    udp.stop();
}

void handleDiscoveryPacket(const String& message) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.println("Error parsing discovery packet: " + String(error.c_str()));
        return;
    }
    
    String type = doc["type"] | "";
    String senderDeviceID = doc["device_id"] | "";
    int senderMode = doc["mode"] | -1;
    String senderIP = doc["ip_address"] | "";
    
    // Only process sender discovery packets if we're an unpaired receiver
    if (currentMode == MODE_RELAY_RECEIVER && !isPaired &&
        type == "discovery" && senderMode == MODE_DRY_CONTACT_SENDER &&
        senderDeviceID.length() > 0 && senderIP.length() > 0) {
        
        Serial.println("Found sender: " + senderDeviceID + " at " + senderIP);
        pairWithDevice(senderDeviceID, senderIP);
    }
}

void pairWithDevice(const String& deviceID, const String& deviceIP) {
    if (isPaired) {
        Serial.println("Already paired, ignoring new pairing request");
        return;
    }

    isPaired = true;
    pairedDeviceID = deviceID;
    pairedDeviceIP = deviceIP;
    receiverIP = deviceIP; // Update the receiver IP for sending commands
    
    savePairingSettings();
    
    Serial.println("Paired with sender: " + deviceID + " at " + deviceIP);
    
    // Update display
    if (display.isAvailable()) {
        display.setRemoteIP(deviceIP);
    }
    
    // Send pairing confirmation
    JsonDocument doc;
    doc["type"] = "pairing_confirmation";
    doc["device_id"] = deviceID;
    doc["paired_with"] = pairedDeviceID;
    doc["paired_ip"] = pairedDeviceIP;
    doc["timestamp"] = millis();

    String message;
    serializeJson(doc, message);

    udp.beginPacket(pairedDeviceIP.c_str(), DEVICE_DISCOVERY_PORT);
    udp.print(message);
    udp.endPacket();

    Serial.println("Sent pairing confirmation to " + pairedDeviceIP);
}

void unpairDevice() {
    if (!isPaired) {
        Serial.println("No device paired to unpair");
        return;
    }

    Serial.println("Unpairing device: " + pairedDeviceID);

    // Send unpairing notification
    JsonDocument doc;
    doc["type"] = "unpairing";
    doc["device_id"] = deviceID;
    doc["unpairing_from"] = pairedDeviceID;
    doc["timestamp"] = millis();

    String message;
    serializeJson(doc, message);

    udp.beginPacket(pairedDeviceIP.c_str(), DEVICE_DISCOVERY_PORT);
    udp.print(message);
    udp.endPacket();

    // Clear pairing state
    isPaired = false;
    pairedDeviceID = "";
    pairedDeviceIP = "";
    receiverIP = "";
    
    savePairingSettings();
    
    // Update display
    if (display.isAvailable()) {
        display.setRemoteIP("");
    }
    
    Serial.println("Device unpaired successfully");
}

void loadPairingSettings() {
    Preferences preferences;
    preferences.begin("pairing", false);
    
    isPaired = preferences.getBool("is_paired", false);
    pairedDeviceID = preferences.getString("paired_device_id", "");
    pairedDeviceIP = preferences.getString("paired_device_ip", "");

    // Migrate from the older per-channel keys so a device paired on previous
    // firmware keeps its pairing across the upgrade to device-to-device pairing.
    if (!isPaired && preferences.getBool("is_paired_ch1", false)) {
        isPaired = true;
        pairedDeviceID = preferences.getString("paired_device_id_ch1", "");
        pairedDeviceIP = preferences.getString("paired_device_ip_ch1", "");
    }

    if (isPaired && pairedDeviceID.length() > 0 && pairedDeviceIP.length() > 0) {
        receiverIP = pairedDeviceIP; // Update receiver IP for communication
        Serial.println("Loaded pairing settings:");
        Serial.println("  Paired with: " + pairedDeviceID);
        Serial.println("  Paired IP: " + pairedDeviceIP);
    } else {
        isPaired = false;
        Serial.println("No pairing settings found");
    }

    preferences.end();
}

void savePairingSettings() {
    Preferences preferences;
    preferences.begin("pairing", false);
    
    preferences.putBool("is_paired", isPaired);
    preferences.putString("paired_device_id", pairedDeviceID);
    preferences.putString("paired_device_ip", pairedDeviceIP);

    // Remove stale per-channel keys left by older firmware.
    preferences.remove("is_paired_ch1");
    preferences.remove("is_paired_ch2");
    preferences.remove("paired_device_id_ch1");
    preferences.remove("paired_device_id_ch2");
    preferences.remove("paired_device_ip_ch1");
    preferences.remove("paired_device_ip_ch2");
    
    preferences.end();
    
    Serial.println("Pairing settings saved");
}

void loadChannelNames() {
    Preferences preferences;
    preferences.begin("channel_names", false);
    
    channel1Name = preferences.getString("channel1_name", DEFAULT_CHANNEL1_NAME);
    channel2Name = preferences.getString("channel2_name", DEFAULT_CHANNEL2_NAME);
    
    // Validate names (ensure they're one word and not too long)
    channel1Name.trim();
    channel2Name.trim();
    
    if (channel1Name.length() > MAX_CHANNEL_NAME_LENGTH || channel1Name.indexOf(' ') >= 0) {
        channel1Name = DEFAULT_CHANNEL1_NAME;
    }
    if (channel2Name.length() > MAX_CHANNEL_NAME_LENGTH || channel2Name.indexOf(' ') >= 0) {
        channel2Name = DEFAULT_CHANNEL2_NAME;
    }
    
    preferences.end();
    Serial.println("Channel names loaded: CH1='" + channel1Name + "' CH2='" + channel2Name + "'");
}

void saveChannelNames() {
    Preferences preferences;
    preferences.begin("channel_names", false);
    
    preferences.putString("channel1_name", channel1Name);
    preferences.putString("channel2_name", channel2Name);
    
    preferences.end();
    Serial.println("Channel names saved: CH1='" + channel1Name + "' CH2='" + channel2Name + "'");
    
    // Update display with new channel names
    if (display.isAvailable()) {
        display.setChannelNames(channel1Name, channel2Name);
    }
}

void checkFactoryReset() {
    Serial.println("Checking for factory reset button during boot...");
    
    // Check if factory reset button is pressed during boot
    bool buttonPressed = (digitalRead(FACTORY_RESET_PIN) == LOW);
    
    if (buttonPressed) {
        Serial.println("Factory reset button detected during boot - checking hold time...");
        
        unsigned long startTime = millis();
        bool stillPressed = true;
        
        // Check if button is held for the required time
        while (stillPressed && (millis() - startTime < FACTORY_RESET_BOOT_DETECTION_TIME_MS)) {
            stillPressed = (digitalRead(FACTORY_RESET_PIN) == LOW);
            delay(10);
        }
        
        if (stillPressed) {
            Serial.println("Factory reset button held for " + String(FACTORY_RESET_BOOT_DETECTION_TIME_MS / 1000) + 
                          " seconds during boot - performing factory reset!");
            performFactoryReset();
        } else {
            Serial.println("Factory reset button released too early - continuing normal boot");
        }
    }
}

void handleFactoryResetButton() {
    bool buttonPressed = (digitalRead(FACTORY_RESET_PIN) == LOW);
    
    if (buttonPressed && !factoryResetButtonPressed) {
        // Button just pressed
        factoryResetButtonPressed = true;
        factoryResetButtonPressTime = millis();
        Serial.println("Factory reset button pressed - hold for " + String(FACTORY_RESET_HOLD_TIME_MS / 1000) + " seconds to reboot");
        
    } else if (!buttonPressed && factoryResetButtonPressed) {
        // Button released
        factoryResetButtonPressed = false;
        unsigned long holdTime = millis() - factoryResetButtonPressTime;
        
        if (holdTime >= FACTORY_RESET_HOLD_TIME_MS) {
            Serial.println("Factory reset button held for " + String(holdTime / 1000) + " seconds - rebooting!");
            delay(100); // Brief delay for serial output
            
            // Simple reboot (no factory reset)
            ESP.restart();
        } else {
            Serial.println("Factory reset button released after " + String(holdTime / 1000) + " seconds - no action");
        }
    }
}

void performFactoryReset() {
    Serial.println("========================================");
    Serial.println("PERFORMING FACTORY RESET");
    Serial.println("========================================");
    
    // Clear all preferences
    Preferences preferences;
    
    // Clear configuration
    preferences.begin("config", false);
    preferences.clear();
    preferences.end();
    
    // Clear pairing settings
    preferences.begin("pairing", false);
    preferences.clear();
    preferences.end();
    
    // Clear channel names
    preferences.begin("channel_names", false);
    preferences.clear();
    preferences.end();
    
    // Clear state logger
    preferences.begin("state_log", false);
    preferences.clear();
    preferences.end();
    
    Serial.println("All settings cleared - rebooting...");
    
    // Blink LED to indicate factory reset
    if (display.isAvailable()) {
        display.showFactoryReset();
        delay(2000);
    }
    
    // Blink status LED
    for (int i = 0; i < 5; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(200);
        digitalWrite(STATUS_LED_PIN, LOW);
        delay(200);
    }
    
    delay(1000);
    ESP.restart();
}
